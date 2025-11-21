#pragma once

#include <JuceHeader.h>
#include <string>

namespace TriggeredAverage
{

/**
 * Simple RAII-based performance timer for profiling code sections.
 * Usage:
 *   {
 *       PerformanceTimer timer("MyFunction");
 *       // ... code to profile ...
 *   } // Timer automatically logs duration when it goes out of scope
 */
class PerformanceTimer
{
public:
    PerformanceTimer(const String& label, double thresholdMs = 0.0)
        : m_label(label)
        , m_thresholdMs(thresholdMs)
        , m_startTime(Time::getHighResolutionTicks())
    {
    }

    ~PerformanceTimer()
    {
        auto endTime = Time::getHighResolutionTicks();
        auto durationMs = Time::highResolutionTicksToSeconds(endTime - m_startTime) * 1000.0;

        if (durationMs >= m_thresholdMs)
        {
            DBG(m_label + " took " + String(durationMs, 2) + " ms");
        }
    }

    // Get elapsed time without destroying the timer
    double getElapsedMs() const
    {
        auto currentTime = Time::getHighResolutionTicks();
        return Time::highResolutionTicksToSeconds(currentTime - m_startTime) * 1000.0;
    }

private:
    String m_label;
    double m_thresholdMs;
    int64 m_startTime;
};

/**
 * Accumulating performance statistics for repeated operations.
 * Useful for profiling operations that happen many times (e.g., paint calls)
 */
class PerformanceStats
{
public:
    PerformanceStats(const String& label)
        : m_label(label)
        , m_count(0)
        , m_totalMs(0.0)
        , m_minMs(std::numeric_limits<double>::max())
        , m_maxMs(0.0)
    {
    }

    void addSample(double durationMs)
    {
        m_count++;
        m_totalMs += durationMs;
        m_minMs = std::min(m_minMs, durationMs);
        m_maxMs = std::max(m_maxMs, durationMs);

        // Log summary every 100 samples
        if (m_count % 100 == 0)
        {
            logSummary();
        }
    }

    void logSummary() const
    {
        if (m_count > 0)
        {
            double avgMs = m_totalMs / m_count;
            DBG(m_label + " stats: count=" + String(m_count) 
                + ", avg=" + String(avgMs, 2) + "ms"
                + ", min=" + String(m_minMs, 2) + "ms"
                + ", max=" + String(m_maxMs, 2) + "ms");
        }
    }

    void reset()
    {
        m_count = 0;
        m_totalMs = 0.0;
        m_minMs = std::numeric_limits<double>::max();
        m_maxMs = 0.0;
    }

private:
    String m_label;
    int m_count;
    double m_totalMs;
    double m_minMs;
    double m_maxMs;
};

} // namespace TriggeredAverage
