#include "stdafx.h"
#include "Data/DataHandler/ModelLoadTimer.h"

ModelLoadTimer::ModelLoadTimer(const wchar_t* what, int model)
    : m_what(what), m_model(model), m_start(std::chrono::steady_clock::now())
{
}

ModelLoadTimer::~ModelLoadTimer()
{
    const std::chrono::duration<double, std::milli> elapsed = std::chrono::steady_clock::now() - m_start;
    if (elapsed.count() < ReportThresholdMs)
    {
        return;
    }

    g_ErrorReport.Write(L"[LoadTime] %ls model=%d: %.1f ms\r\n", m_what, m_model, elapsed.count());
}
