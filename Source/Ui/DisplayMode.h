// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>

namespace TriggeredAverage
{

enum class DisplayMode : std::uint8_t
{
    INVALID = 0,
    INDIVIDUAL_TRACES = 1,
    AVERAGE_TRAGE = 2,
    ALL_AND_AVERAGE = 3,
};

constexpr auto DisplayModeModeToString (DisplayMode mode) -> const char*
{
    // using enum DisplayMode;
    switch (mode)
    {
        case DisplayMode::INVALID:
            return "Invalid";
        case DisplayMode::INDIVIDUAL_TRACES:
            return "All traces";
        case DisplayMode::AVERAGE_TRAGE:
            return "Average trace";
        case DisplayMode::ALL_AND_AVERAGE:
            return "Average + All";
        default:
            return "Unknown";
    }
}

} // namespace TriggeredAverage
