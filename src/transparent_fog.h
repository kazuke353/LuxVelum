#pragma once

#include "engine.h"
#include "fog_model.h"

#include <cstdint>

enum class FogPass
{
    WholeFrame,
    BeforeTransparents,
};

struct StockFogFitLight
{
    float exposure;
    float glowToCompensate;
    uint32_t uploadedPointLights = 0;
    float pointLightPhase = 0.0f;
};

struct ObjectFogScene
{
    bool valid = false;
    FogParams fog = {};
    FrameInputs in = {};
    StockFogFitLight light = {};
};

struct StockFogFit
{
    bool fogs = false;
    float start = 0.0f;
    float end = 0.0f;
    uint32_t colour = 0;
    ObjectFogScene scene;
};

struct ObjectFog
{
    bool fogs = false;
    float start = 0.0f;
    float end = 0.0f;
    uint32_t colour = 0;
    float viewDepth = 0.0f;
    float transmittance = 1.0f;
    float scatterPerOpacity[3] = {};
    float displayOpacity = 0.0f;
};

struct M2BatchFogArgs
{
    float start;
    float end;
    float exponent;
    const uint32_t* colour;
};

constexpr float kStockFogFitDepth = 100.0f;
constexpr float kLinearStockFogExponent = 1.0f;
constexpr uint32_t kLightingFogColourAlpha = 0xFF000000u;
constexpr uint32_t kFogColourAlphaMask = 0xFF000000u;
constexpr uint32_t kAdditiveFogColour = 0x00000000u;
constexpr uint32_t kModulateFogColour = 0x00FFFFFFu;
constexpr uint32_t kModulate2xFogColour = 0x00808080u;
constexpr uint16_t kM2BlendModes = 8;

constexpr float kObjectFogMinViewDepth = 0.5f;
constexpr float kUnfoggedObjectStart = 100000.0f;
constexpr float kObjectPlacementTolerance = 0.02f;
constexpr float kObjectPlacementMinTolerance = 1.0f;

StockFogFit FitStockFog(const FogParams& fog, const FrameInputs& in, const StockFogFitLight& light);
bool UsesLightingFogColour(uint32_t colour);
void ApplyStockFogFit(const StockFogFit& fit, M2BatchFogArgs& args);
bool UnfoggedBatchFogColour(uint16_t blendMode, uint32_t& colour);
bool PlacementMatchesCamera(const float* world, float cameraDistanceSquared, const float* camPos);
bool FitObjectFog(const StockFogFit& fit, const float* world, ObjectFog& out);
void ApplyObjectFog(const ObjectFog& fog, M2BatchFogArgs& args);
