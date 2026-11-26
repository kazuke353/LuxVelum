#pragma once

#include "client_code.h"

#include <cstdint>

constexpr uintptr_t kLightIntBandTable = 0x00AF49BC;
constexpr uintptr_t kLightIntBandTableVtable = 0x00A419E0;

namespace engine
{
bool ClientIntBandsLoaded(const ClientCodeView& memory);
bool ReadClientIntBandValue(const ClientCodeView& memory, uint32_t bandId, uint32_t keyIndex, uint32_t& rgb);
}
