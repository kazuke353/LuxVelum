#include "client_light_bands.h"

namespace
{
constexpr uintptr_t kTableLoaded = 0x04;
constexpr uintptr_t kTableMaxId = 0x0C;
constexpr uintptr_t kTableMinId = 0x10;
constexpr uintptr_t kTableRecordIndex = 0x20;
constexpr uintptr_t kBandRecordId = 0x00;
constexpr uintptr_t kBandRecordKeyCount = 0x04;
constexpr uintptr_t kBandRecordValues = 0x48;
constexpr uint32_t kBandKeys = 16;
constexpr uint32_t kRgbMask = 0xFFFFFF;

template <typename T>
bool ReadValue(const ClientCodeView& memory, uintptr_t address, T& value)
{
    return engine::ReadClientCode(memory, address, reinterpret_cast<unsigned char*>(&value), sizeof(value));
}
}

namespace engine
{
bool ClientIntBandsLoaded(const ClientCodeView& memory)
{
    uint32_t vtable = 0;
    uint32_t loaded = 0;
    return ReadValue(memory, kLightIntBandTable, vtable) && vtable == kLightIntBandTableVtable &&
           ReadValue(memory, kLightIntBandTable + kTableLoaded, loaded) && loaded != 0;
}

bool ReadClientIntBandValue(const ClientCodeView& memory, uint32_t bandId, uint32_t keyIndex, uint32_t& rgb)
{
    int32_t minId = 0;
    int32_t maxId = 0;
    uint32_t index = 0;
    if (keyIndex >= kBandKeys || !ClientIntBandsLoaded(memory) ||
        !ReadValue(memory, kLightIntBandTable + kTableMinId, minId) ||
        !ReadValue(memory, kLightIntBandTable + kTableMaxId, maxId) ||
        !ReadValue(memory, kLightIntBandTable + kTableRecordIndex, index) || index == 0)
        return false;
    const int64_t id = bandId;
    if (id < minId || id > maxId)
        return false;
    uint32_t record = 0;
    uint32_t recordId = 0;
    uint32_t keyCount = 0;
    uint32_t value = 0;
    if (!ReadValue(memory, index + static_cast<uintptr_t>(id - minId) * sizeof(uint32_t), record) || record == 0 ||
        !ReadValue(memory, record + kBandRecordId, recordId) || recordId != bandId ||
        !ReadValue(memory, record + kBandRecordKeyCount, keyCount) || keyIndex >= keyCount ||
        !ReadValue(memory, record + kBandRecordValues + keyIndex * sizeof(uint32_t), value))
        return false;
    rgb = value & kRgbMask;
    return true;
}
}
