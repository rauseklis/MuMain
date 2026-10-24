#pragma once

#include <chrono>

// Writes a line to the error report when loading a model or its textures blocks the game thread
// for at least ModelLoadTimer::ReportThresholdMs. Used to find what makes the game stutter when
// an NPC or monster appears for the first time.
class ModelLoadTimer
{
public:
    ModelLoadTimer(const wchar_t* what, int model);
    ~ModelLoadTimer();

    ModelLoadTimer(const ModelLoadTimer&) = delete;
    ModelLoadTimer& operator=(const ModelLoadTimer&) = delete;

    static constexpr double ReportThresholdMs = 2.0;

private:
    const wchar_t* m_what;
    int m_model;
    std::chrono::steady_clock::time_point m_start;
};
