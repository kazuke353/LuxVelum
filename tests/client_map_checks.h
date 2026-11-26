#pragma once

namespace client_map_checks
{
constexpr size_t kSetterCodeBytes = 0x20;
constexpr unsigned char kSetterFiller = 0xCC;
constexpr unsigned char kMoveEaxFromMemory = 0xA1;
constexpr unsigned char kPushImmediate = 0x68;

struct SetterCode
{
    unsigned char bytes[kSetterCodeBytes] = {};
    uintptr_t base = kFarClipSetterMapRead - 3;

    SetterCode() { std::memset(bytes, kSetterFiller, sizeof(bytes)); }
    ClientCodeView View() const { return {base, bytes, sizeof(bytes)}; }
    void WriteMapRead(unsigned char opcode, uintptr_t source)
    {
        unsigned char* read = bytes + (kFarClipSetterMapRead - base);
        read[0] = opcode;
        const uint32_t address = static_cast<uint32_t>(source);
        std::memcpy(read + 1, &address, sizeof(address));
    }
};

void CheckCurrentMapFollowsTheFarClipSetter()
{
    SetterCode stock;
    stock.WriteMapRead(kMoveEaxFromMemory, kWorldMap);
    SetterCode otherSource;
    otherSource.WriteMapRead(kMoveEaxFromMemory, kLoadingScreenMap);
    SetterCode otherOpcode;
    otherOpcode.WriteMapRead(kPushImmediate, kWorldMap);
    const ClientCodeView cut = {stock.base, stock.bytes, kFarClipSetterMapRead - stock.base + 2};
    Check(engine::CurrentMapAddress(stock.View()) == kWorldMap &&
              engine::CurrentMapAddress(otherSource.View()) == kLoadingScreenMap &&
              engine::CurrentMapAddress(otherOpcode.View()) == kLoadingScreenMap &&
              engine::CurrentMapAddress(cut) == kLoadingScreenMap,
          "the map id comes from the world map the far clip setter reads (mov eax,[0xADFBC4] at 0x780803), which "
          "survives ship and zeppelin trips; any other code there falls back to the loading screen's map");
}
}
