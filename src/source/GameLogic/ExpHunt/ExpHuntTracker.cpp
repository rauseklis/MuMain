// client/src/source/GameLogic/ExpHunt/ExpHuntTracker.cpp
#include "stdafx.h"
#include "GameLogic/ExpHunt/ExpHuntTracker.h"

#include <deque>

extern double WorldTime;

namespace GameLogic::ExpHunt
{
    namespace
    {
        constexpr double kWindowMs = 60000.0;

        struct Sample
        {
            double timestampMs;
            std::uint64_t experience;
        };

        bool s_active = false;
        double s_startTimeMs = 0.0;
        std::deque<Sample> s_samples;

        void PurgeOldSamples()
        {
            const double cutoff = WorldTime - kWindowMs;
            while (!s_samples.empty() && s_samples.front().timestampMs < cutoff)
            {
                s_samples.pop_front();
            }
        }
    }

    void Start()
    {
        s_active = true;
        s_startTimeMs = WorldTime;
        s_samples.clear();
    }

    void Stop()
    {
        s_active = false;
        s_samples.clear();
    }

    bool IsActive()
    {
        return s_active;
    }

    void AddSample(std::uint64_t experience)
    {
        if (!s_active)
        {
            return;
        }

        s_samples.push_back({ WorldTime, experience });
    }

    double GetRatePerMinute()
    {
        if (!s_active)
        {
            return 0.0;
        }

        PurgeOldSamples();

        const double elapsed = WorldTime - s_startTimeMs;
        const double windowMs = elapsed < kWindowMs ? elapsed : kWindowMs;
        if (windowMs <= 0.0)
        {
            return 0.0;
        }

        std::uint64_t total = 0;
        for (const auto& sample : s_samples)
        {
            total += sample.experience;
        }

        return static_cast<double>(total) * (kWindowMs / windowMs);
    }
}
