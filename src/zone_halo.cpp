#include "zone_halo.h"

#include "log.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{
constexpr const char* kSunSection = "SunHalo";
constexpr const char* kMoonSection = "MoonHalo";
constexpr const char* kDensitySection = "Density";
constexpr const char* kNameSection = "Zones";
constexpr DWORD kSectionBufferSize = 64 * 1024;
constexpr float kSettledDifference = 0.001f;

float ClampedFactor(float value)
{
    return std::isfinite(value) ? std::clamp(value, 0.0f, kZoneHaloMax) : 1.0f;
}

template <typename Use>
void ForEachEntry(const std::string& path, const char* section, Use use)
{
    std::vector<char> buffer(kSectionBufferSize);
    const DWORD length = GetPrivateProfileSectionA(section, buffer.data(), kSectionBufferSize, path.c_str());
    for (const char* entry = buffer.data(); entry < buffer.data() + length && *entry; entry += std::strlen(entry) + 1)
    {
        const char* equals = std::strchr(entry, '=');
        if (!equals)
            continue;
        char* end = nullptr;
        const long zoneId = std::strtol(entry, &end, 10);
        if (end == entry || zoneId <= 0)
            continue;
        use(static_cast<int>(zoneId), static_cast<float>(std::atof(equals + 1)));
    }
}

bool WriteFactor(const std::string& path, const char* section, int zoneId, float value)
{
    char key[16];
    char text[16];
    std::snprintf(key, sizeof(key), "%d", zoneId);
    std::snprintf(text, sizeof(text), "%.2f", value);
    return WritePrivateProfileStringA(section, key, text, path.c_str()) != 0;
}

bool EraseKey(const std::string& path, const char* section, int zoneId)
{
    char key[16];
    std::snprintf(key, sizeof(key), "%d", zoneId);
    return WritePrivateProfileStringA(section, key, nullptr, path.c_str()) != 0;
}

float Approach(float current, float target, double elapsed)
{
    return current + (target - current) * static_cast<float>(1.0 - std::exp(-elapsed / kZoneHaloFadeSeconds));
}
}

void ZoneHaloTable::Load(const std::string& path)
{
    m_path = path;
    m_factors.clear();
    ForEachEntry(path, kSunSection, [&](int zoneId, float value) { m_factors[zoneId].sun = ClampedFactor(value); });
    ForEachEntry(path, kMoonSection, [&](int zoneId, float value) { m_factors[zoneId].moon = ClampedFactor(value); });
    ForEachEntry(path, kDensitySection,
                 [&](int zoneId, float value) { m_factors[zoneId].density = ClampedFactor(value); });
}

ZoneHaloFactors ZoneHaloTable::Factors(int zoneId) const
{
    const auto found = m_factors.find(zoneId);
    return found == m_factors.end() ? ZoneHaloFactors{} : found->second;
}

void ZoneHaloTable::Edit(int zoneId, const ZoneHaloFactors& factors)
{
    if (zoneId > 0)
        m_factors[zoneId] = {ClampedFactor(factors.sun), ClampedFactor(factors.moon), ClampedFactor(factors.density)};
}

bool ZoneHaloTable::Save(int zoneId, const char* zoneName) const
{
    if (zoneId <= 0 || m_path.empty())
        return false;
    const ZoneHaloFactors factors = Factors(zoneId);
    char key[16];
    std::snprintf(key, sizeof(key), "%d", zoneId);
    const bool named = !zoneName || !*zoneName ||
                       WritePrivateProfileStringA(kNameSection, key, zoneName, m_path.c_str()) != 0;
    return WriteFactor(m_path, kSunSection, zoneId, factors.sun) &&
           WriteFactor(m_path, kMoonSection, zoneId, factors.moon) &&
           WriteFactor(m_path, kDensitySection, zoneId, factors.density) && named;
}

bool ZoneHaloTable::Forget(int zoneId)
{
    m_factors.erase(zoneId);
    if (m_path.empty())
        return false;
    const bool sun = EraseKey(m_path, kSunSection, zoneId);
    const bool moon = EraseKey(m_path, kMoonSection, zoneId);
    const bool density = EraseKey(m_path, kDensitySection, zoneId);
    return EraseKey(m_path, kNameSection, zoneId) && sun && moon && density;
}

ZoneHalo ZoneHaloFade::Advance(int zoneId, const ZoneHaloFactors& target, double seconds)
{
    if (!m_started)
    {
        m_started = true;
        m_advancedTo = seconds;
        m_current = {zoneId, target.sun, target.moon, target.density};
        return m_current;
    }
    const double elapsed = std::max(seconds - m_advancedTo, 0.0);
    m_advancedTo = seconds;
    if (zoneId != m_current.zoneId)
        m_fading = true;
    m_current.zoneId = zoneId;
    if (!m_fading)
    {
        m_current.sun = target.sun;
        m_current.moon = target.moon;
        m_current.density = target.density;
        return m_current;
    }
    m_current.sun = Approach(m_current.sun, target.sun, elapsed);
    m_current.moon = Approach(m_current.moon, target.moon, elapsed);
    m_current.density = Approach(m_current.density, target.density, elapsed);
    m_fading = std::fabs(m_current.sun - target.sun) > kSettledDifference ||
               std::fabs(m_current.moon - target.moon) > kSettledDifference ||
               std::fabs(m_current.density - target.density) > kSettledDifference;
    return m_current;
}

static const char* SourceNote(ZoneHaloSource source)
{
    switch (source)
    {
    case ZoneHaloSource::Own:
        return " (your own)";
    case ZoneHaloSource::Shipped:
        return " (shipped)";
    case ZoneHaloSource::Unlisted:
        break;
    }
    return " (not listed)";
}

void ZoneHaloBook::Load(const std::string& shippedPath, const std::string& ownPath)
{
    m_shipped.Load(shippedPath);
    m_own.Load(ownPath);
    VF_LOG_INFO("zone halo: %d zone(s) shipped in %s, %d of your own in %s", m_shipped.Count(), shippedPath.c_str(),
                m_own.Count(), ownPath.c_str());
}

ZoneHaloFactors ZoneHaloBook::Factors(int zoneId) const
{
    return m_own.Listed(zoneId) ? m_own.Factors(zoneId) : m_shipped.Factors(zoneId);
}

ZoneHaloSource ZoneHaloBook::SourceOf(int zoneId) const
{
    if (m_own.Listed(zoneId))
        return ZoneHaloSource::Own;
    return m_shipped.Listed(zoneId) ? ZoneHaloSource::Shipped : ZoneHaloSource::Unlisted;
}

void ZoneHaloBook::Show(int zoneId)
{
    m_view.factors = Factors(zoneId);
    m_view.source = SourceOf(zoneId);
}

ZoneHalo ZoneHaloBook::Advance(int zoneId, const char* zoneName, double seconds)
{
    if (zoneId <= 0 && m_view.zoneId > 0)
        return m_fade.Advance(m_view.zoneId, Factors(m_view.zoneId), seconds);
    if (zoneId != m_view.zoneId)
    {
        m_view.zoneId = zoneId;
        std::snprintf(m_view.zoneName, sizeof(m_view.zoneName), "%s", zoneName ? zoneName : "");
        const ZoneHaloFactors target = Factors(zoneId);
        VF_LOG_INFO("zone %d (%s): sun halo %.2f, moon halo %.2f, fog density %.2f%s", zoneId, m_view.zoneName,
                    target.sun, target.moon, target.density, SourceNote(SourceOf(zoneId)));
    }
    Show(zoneId);
    return m_fade.Advance(zoneId, m_view.factors, seconds);
}

ZoneHaloView ZoneHaloBook::Current() const
{
    return m_view;
}

void ZoneHaloBook::Edit(const ZoneHaloFactors& factors)
{
    m_own.Edit(m_view.zoneId, factors);
    Show(m_view.zoneId);
}

bool ZoneHaloBook::SaveCurrent()
{
    const bool saved = m_own.Save(m_view.zoneId, m_view.zoneName);
    const ZoneHaloFactors factors = m_own.Factors(m_view.zoneId);
    VF_LOG_INFO("zone %d (%s): your values saved as sun halo %.2f, moon halo %.2f, fog density %.2f%s", m_view.zoneId,
                m_view.zoneName, factors.sun, factors.moon, factors.density, saved ? "" : " (write failed)");
    return saved;
}

bool ZoneHaloBook::ForgetCurrent()
{
    const bool forgotten = m_own.Forget(m_view.zoneId);
    Show(m_view.zoneId);
    VF_LOG_INFO("zone %d (%s): your values removed, back to sun halo %.2f, moon halo %.2f, fog density %.2f%s%s",
                m_view.zoneId, m_view.zoneName, m_view.factors.sun, m_view.factors.moon, m_view.factors.density,
                SourceNote(m_view.source), forgotten ? "" : " (write failed)");
    return forgotten;
}

ZoneHaloBook& GlobalZoneHalo()
{
    static ZoneHaloBook book;
    return book;
}
