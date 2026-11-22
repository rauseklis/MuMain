#include "stdafx.h"

#include "Network/Server/ClientIntegrity.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include <openssl/evp.h>

#include "Network/Server/WSclient.h"

extern unsigned char Serial[];

namespace
{
constexpr std::size_t BuildLength = 16;
constexpr std::size_t HashLength = 32;
constexpr int MaximumReportsPerConnection = 2;

using Hash = std::array<std::uint8_t, HashLength>;

class Sha256
{
public:
    Sha256() : m_context(EVP_MD_CTX_new())
    {
        if (m_context != nullptr && EVP_DigestInit_ex(m_context, EVP_sha256(), nullptr) != 1)
        {
            EVP_MD_CTX_free(m_context);
            m_context = nullptr;
        }
    }

    ~Sha256()
    {
        if (m_context != nullptr)
        {
            EVP_MD_CTX_free(m_context);
        }
    }

    Sha256(const Sha256&) = delete;
    Sha256& operator=(const Sha256&) = delete;

    bool Update(const void* data, std::size_t size)
    {
        return m_context != nullptr && EVP_DigestUpdate(m_context, data, size) == 1;
    }

    std::optional<Hash> Finish()
    {
        Hash result{};
        unsigned int length = 0;
        if (m_context == nullptr || EVP_DigestFinal_ex(m_context, result.data(), &length) != 1 || length != HashLength)
        {
            return std::nullopt;
        }
        return result;
    }

private:
    EVP_MD_CTX* m_context;
};

std::string ToHex(const Hash& hash)
{
    static const char digits[] = "0123456789abcdef";
    std::string text;
    for (const std::uint8_t value : hash)
    {
        text.push_back(digits[value >> 4]);
        text.push_back(digits[value & 0x0F]);
    }
    return text;
}

std::optional<Hash> HashFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return std::nullopt;
    }

    Sha256 sha;
    std::vector<char> buffer(1 << 16);
    while (stream)
    {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = stream.gcount();
        if (count > 0 && !sha.Update(buffer.data(), static_cast<std::size_t>(count)))
        {
            return std::nullopt;
        }
    }
    if (stream.bad())
    {
        return std::nullopt;
    }
    return sha.Finish();
}

std::string LowerName(const std::filesystem::path& path)
{
    std::string name = path.filename().string();
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name;
}

// The fingerprint: the SHA-256 of the lines "<lowercase file name>:<lowercase SHA-256 of the file>\n" for the
// executable and every DLL in its folder, sorted by name. The release list (client-manifest.json) holds the same file
// hashes, so the expected value of a release can be computed from it.
std::optional<Hash> ComputeProgramHash()
{
#ifdef _WIN32
    try
    {
        wchar_t modulePath[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
        {
            return std::nullopt;
        }

        const std::filesystem::path executable(modulePath);
        std::vector<std::filesystem::path> files{executable};
        for (const auto& entry : std::filesystem::directory_iterator(executable.parent_path()))
        {
            if (entry.is_regular_file() && LowerName(entry.path()).ends_with(".dll"))
            {
                files.push_back(entry.path());
            }
        }

        std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) { return LowerName(a) < LowerName(b); });

        Sha256 total;
        for (const auto& file : files)
        {
            const auto fileHash = HashFile(file);
            if (!fileHash)
            {
                return std::nullopt;
            }
            const std::string line = LowerName(file) + ":" + ToHex(*fileHash) + "\n";
            if (!total.Update(line.data(), line.size()))
            {
                return std::nullopt;
            }
        }
        return total.Finish();
    }
    catch (...)
    {
        return std::nullopt;
    }
#else
    return std::nullopt;
#endif
}

std::future<std::optional<Hash>> g_programHash;
bool g_programHashStarted = false;
std::optional<Hash> g_result;
int g_reportsSent = 0;
} // namespace

namespace ClientIntegrity
{
void OnTick(bool loggedIn)
{
    if (!loggedIn || g_reportsSent >= MaximumReportsPerConnection)
    {
        return;
    }

    try
    {
        if (!g_programHashStarted)
        {
            g_programHashStarted = true;
            g_programHash = std::async(std::launch::async, ComputeProgramHash);
            return;
        }

        if (g_programHash.valid())
        {
            if (g_programHash.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            {
                return;
            }
            g_result = g_programHash.get();
        }

        if (!g_result)
        {
            return;
        }

        auto* sender = SocketClient != nullptr ? SocketClient->ToGameServer() : nullptr;
        if (sender == nullptr)
        {
            return;
        }

        sender->SendClientIntegrityReport(Serial, static_cast<std::uint32_t>(BuildLength), g_result->data(),
                                          static_cast<std::uint32_t>(HashLength));
        ++g_reportsSent;
    }
    catch (...)
    {
        g_reportsSent = MaximumReportsPerConnection;
    }
}

void OnDisconnected()
{
    g_reportsSent = 0;
}
} // namespace ClientIntegrity
