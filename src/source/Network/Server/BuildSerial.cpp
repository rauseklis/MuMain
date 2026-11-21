// The serial field of the login names the build: it holds the first characters of the git commit this client was
// built from. The last character is 'x' when the source differed from that commit, and the text is "unknown" when
// there was no commit. The server records the serial, so it knows which build connects. It is a label of the build,
// not a secret and not a proof: whoever builds their own client can send anything.
//
// This file is compiled straight into the Main executable (see src/CMakeLists.txt) and not into the MuClient
// library: nothing references it, and the linker would drop an unreferenced object file of a static library together
// with the initializer below.

#include <cstddef>
#include <cstring>

#include "MuGitCommit.h"

namespace
{
constexpr std::size_t SerialLength = 16;
} // namespace

// Defined in Network/Server/WSclient.cpp (BYTE Serial[SIZE_PROTOCOLSERIAL + 1]).
extern unsigned char Serial[SerialLength + 1];

namespace
{
const bool g_serialNamesTheBuild = []()
{
    const char* commit = MU_GIT_COMMIT;
    const std::size_t length = std::strlen(commit);
    for (std::size_t i = 0; i < SerialLength; ++i)
    {
        Serial[i] = static_cast<unsigned char>(i < length ? commit[i] : '0');
    }

    if (MU_GIT_COMMIT_CHANGED != 0)
    {
        Serial[SerialLength - 1] = 'x';
    }

    Serial[SerialLength] = 0;
    return true;
}();
} // namespace
