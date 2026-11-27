#include "stdafx.h"
#include "Core/Platform/HangWatchdog.h"

#include <windows.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <thread>

#include "Core/Utilities/Log/MuLogger.h"

namespace Core::Platform::HangWatchdog
{
namespace
{
constexpr std::chrono::seconds POLL_INTERVAL{1};
constexpr int STALL_SECONDS_BEFORE_FIRST_REPORT = 8;
constexpr int SECONDS_BETWEEN_REPORTS = 10;
constexpr int MAX_REPORTS_PER_STALL = 3;
constexpr int MAX_STACK_FRAMES = 48;

std::atomic<std::uint64_t> g_beats{0};
HANDLE g_mainThread = nullptr;
std::once_flag g_startOnce;

struct StackSample
{
    std::array<DWORD64, MAX_STACK_FRAMES> addresses{};
    int count = 0;
};

bool ReadReturnAddress(DWORD64 stackPointer, DWORD64& returnAddress)
{
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(stackPointer), &returnAddress,
                             sizeof(returnAddress), &bytesRead) != FALSE &&
           bytesRead == sizeof(returnAddress);
}

// Walks the stack with the unwind tables only. No allocation and no logging while the thread is suspended,
// so a lock held by the frozen thread cannot block the watchdog.
void WalkStack(CONTEXT& context, StackSample& sample)
{
    while (sample.count < MAX_STACK_FRAMES && context.Rip != 0)
    {
        sample.addresses[sample.count++] = context.Rip;

        DWORD64 imageBase = 0;
        PRUNTIME_FUNCTION function = RtlLookupFunctionEntry(context.Rip, &imageBase, nullptr);
        if (function == nullptr)
        {
            // A leaf function: the return address is on top of the stack.
            DWORD64 returnAddress = 0;
            if (sample.count != 1 || !ReadReturnAddress(context.Rsp, returnAddress))
            {
                return;
            }
            context.Rip = returnAddress;
            context.Rsp += sizeof(DWORD64);
            continue;
        }

        void* handlerData = nullptr;
        DWORD64 establisherFrame = 0;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, context.Rip, function, &context, &handlerData, &establisherFrame,
                         nullptr);
    }
}

bool SampleMainThread(StackSample& sample)
{
    if (SuspendThread(g_mainThread) == static_cast<DWORD>(-1))
    {
        return false;
    }

    CONTEXT context{};
    context.ContextFlags = CONTEXT_FULL;
    if (GetThreadContext(g_mainThread, &context))
    {
        WalkStack(context, sample);
    }

    ResumeThread(g_mainThread);
    return sample.count > 0;
}

void LogFrame(int index, DWORD64 address)
{
    HMODULE module = nullptr;
    const DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
    if (!GetModuleHandleExW(flags, reinterpret_cast<LPCWSTR>(address), &module) || module == nullptr)
    {
        mu::log::Get("core")->error("  #{} 0x{:X} (unknown module)", index, address);
        return;
    }

    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(module, path, MAX_PATH);
    const std::string name = std::filesystem::path(path).filename().string();
    const DWORD64 offset = address - reinterpret_cast<DWORD64>(module);
    mu::log::Get("core")->error("  #{} {}+0x{:X}", index, name, offset);
}

void Report(int stalledSeconds)
{
    StackSample sample;
    if (!SampleMainThread(sample))
    {
        mu::log::Get("core")->error("[Hang] No frame for {} s, the main thread stack could not be read",
                                    stalledSeconds);
        return;
    }

    mu::log::Get("core")->error("[Hang] No frame for {} s. Main thread stack (module+offset):", stalledSeconds);
    for (int i = 0; i < sample.count; ++i)
    {
        LogFrame(i, sample.addresses[i]);
    }
}

bool IsReportDue(int stalledSeconds)
{
    if (stalledSeconds < STALL_SECONDS_BEFORE_FIRST_REPORT)
    {
        return false;
    }
    return (stalledSeconds - STALL_SECONDS_BEFORE_FIRST_REPORT) % SECONDS_BETWEEN_REPORTS == 0;
}

void WatchLoop()
{
    std::uint64_t lastBeats = g_beats.load();
    int stalledSeconds = 0;
    int reports = 0;

    for (;;)
    {
        std::this_thread::sleep_for(POLL_INTERVAL);

        const std::uint64_t beats = g_beats.load();
        if (beats != lastBeats)
        {
            lastBeats = beats;
            stalledSeconds = 0;
            reports = 0;
            continue;
        }

        ++stalledSeconds;
        if (reports < MAX_REPORTS_PER_STALL && IsReportDue(stalledSeconds))
        {
            ++reports;
            Report(stalledSeconds);
        }
    }
}

void StartWatching()
{
    const HANDLE process = GetCurrentProcess();
    const DWORD access = THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION;
    if (!DuplicateHandle(process, GetCurrentThread(), process, &g_mainThread, access, FALSE, 0))
    {
        return;
    }

    std::thread(WatchLoop).detach();
}
} // namespace

void Beat()
{
    std::call_once(g_startOnce, StartWatching);
    g_beats.fetch_add(1, std::memory_order_relaxed);
}
} // namespace Core::Platform::HangWatchdog
