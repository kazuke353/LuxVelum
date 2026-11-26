#pragma once

#include "client_code.h"

#include <cstdint>

constexpr uintptr_t kLoadingScreenMap = 0x00AB63BC;
constexpr uintptr_t kWorldMap = 0x00ADFBC4;
constexpr uintptr_t kFarClipSetterMapRead = 0x00780803;

namespace engine
{
uintptr_t CurrentMapAddress(const ClientCodeView& code);
}
