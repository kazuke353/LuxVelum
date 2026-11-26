#include "client_map.h"

namespace
{
const ClientCodeGuard kFarClipSetterReadsWorldMap = {
    "far clip setter map read", kFarClipSetterMapRead, 5, {0xA1, 0xC4, 0xFB, 0xAD, 0x00}};
}

namespace engine
{
uintptr_t CurrentMapAddress(const ClientCodeView& code)
{
    return ClientGuardMatches(code, kFarClipSetterReadsWorldMap) ? kWorldMap : kLoadingScreenMap;
}
}
