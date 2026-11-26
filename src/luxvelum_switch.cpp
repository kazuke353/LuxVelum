#include "luxvelum_switch.h"

#include <atomic>

namespace
{
constexpr unsigned kRequestedOff = 1u;

std::atomic<unsigned> g_request{0u};
bool g_switchedOff = false;
}

void FlipLuxvelumSwitch()
{
    g_request.fetch_xor(kRequestedOff);
}

void SwitchLuxvelumOn()
{
    g_request.store(0u);
}

bool LuxvelumSwitchFlippedOff()
{
    return (g_request.load() & kRequestedOff) != 0;
}

bool LatchLuxvelumSwitch()
{
    const bool off = LuxvelumSwitchFlippedOff();
    const bool changed = off != g_switchedOff;
    g_switchedOff = off;
    return changed;
}

bool LuxvelumSwitchedOff()
{
    return g_switchedOff;
}
