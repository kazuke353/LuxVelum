#pragma once

inline float WaterGlintStrength(float waterSpecular, float waterMoonGlint, bool lightIsMoon)
{
    return lightIsMoon ? waterSpecular * waterMoonGlint : waterSpecular;
}
