#pragma once

namespace fog_data_choice_checks
{
constexpr uintptr_t kTableMemoryBase = 0x00AF4000;
constexpr size_t kTableMemoryBytes = 0x1000;
constexpr uintptr_t kIndexAddress = kTableMemoryBase + 0x100;
constexpr uintptr_t kFirstRecordAddress = kTableMemoryBase + 0x200;
constexpr size_t kBandRecordBytes = 0x88;
constexpr uint32_t kFirstBandId = 127;
constexpr uint32_t kLastBandId = 129;
constexpr uint32_t kDarkerMidnight = 0x637586;
constexpr uint32_t kBrighterMidnight = 0x96B1CB;

struct ClientTableMemory
{
    unsigned char* bytes = nullptr;

    ClientTableMemory()
    {
        bytes = static_cast<unsigned char*>(VirtualAlloc(nullptr, kTableMemoryBytes, MEM_COMMIT | MEM_RESERVE,
                                                         PAGE_READWRITE));
        if (bytes)
            std::memset(bytes, 0, kTableMemoryBytes);
    }
    ~ClientTableMemory()
    {
        if (bytes)
            VirtualFree(bytes, 0, MEM_RELEASE);
    }
    ClientCodeView View() const { return {kTableMemoryBase, bytes, kTableMemoryBytes}; }
    void Put(uintptr_t address, uint32_t value) { std::memcpy(bytes + (address - kTableMemoryBase), &value, 4); }
    void Fill(uint32_t midnight, bool loaded)
    {
        Put(kLightIntBandTable, kLightIntBandTableVtable);
        Put(kLightIntBandTable + 0x04, loaded ? 1 : 0);
        Put(kLightIntBandTable + 0x0C, kLastBandId);
        Put(kLightIntBandTable + 0x10, kFirstBandId);
        Put(kLightIntBandTable + 0x20, static_cast<uint32_t>(kIndexAddress));
        for (uint32_t id = kFirstBandId; id <= kLastBandId; ++id)
        {
            const uintptr_t record = kFirstRecordAddress + (id - kFirstBandId) * kBandRecordBytes;
            Put(kIndexAddress + (id - kFirstBandId) * 4, id == kFirstBandId + 1 ? 0 : static_cast<uint32_t>(record));
            Put(record, id);
            Put(record + 0x04, 2);
            Put(record + 0x48, midnight + id - kFirstBandId);
            Put(record + 0x4C, 0xFF000000u | 0x123456);
        }
    }
};

void CheckClientIntBandReads()
{
    ClientTableMemory memory;
    if (!memory.bytes)
    {
        Check(false, "client LightIntBand table fixture allocated");
        return;
    }
    memory.Fill(kDarkerMidnight, true);
    uint32_t midnight = 0;
    uint32_t second = 0;
    uint32_t ignored = 0;
    const bool read = engine::ReadClientIntBandValue(memory.View(), kFirstBandId, 0, midnight) &&
                      engine::ReadClientIntBandValue(memory.View(), kFirstBandId, 1, second);
    const bool missingRecord = engine::ReadClientIntBandValue(memory.View(), kFirstBandId + 1, 0, ignored);
    const bool beyondTable = engine::ReadClientIntBandValue(memory.View(), kLastBandId + 1, 0, ignored);
    const bool beyondKeys = engine::ReadClientIntBandValue(memory.View(), kFirstBandId, 2, ignored);
    memory.Fill(kDarkerMidnight, false);
    const bool notLoaded = engine::ReadClientIntBandValue(memory.View(), kFirstBandId, 0, ignored);
    std::printf("     LightIntBand %u: midnight 0x%06X, second key 0x%06X; missing %d, beyond %d/%d, unloaded %d\n",
                kFirstBandId, midnight, second, missingRecord, beyondTable, beyondKeys, notLoaded);
    Check(read && midnight == kDarkerMidnight && second == 0x123456 && !missingRecord && !beyondTable &&
              !beyondKeys && !notLoaded,
          "the client's LightIntBand is read through its table at 0x00AF49BC (id range, record index, key count, "
          "colour without its top byte) and not before the client loaded it");
}

BandReader ReaderOf(uint32_t midnight)
{
    return [midnight](uint32_t bandId, uint32_t keyIndex, uint32_t& rgb) {
        if (bandId < kFirstBandId || bandId > kLastBandId || keyIndex != 0)
            return false;
        rgb = midnight + bandId - kFirstBandId;
        return true;
    };
}

std::vector<FogBandProbe> ProbesOf(uint32_t midnight)
{
    std::vector<FogBandProbe> probes;
    for (uint32_t id = kFirstBandId; id <= kLastBandId; ++id)
        probes.push_back({id, 0, midnight + id - kFirstBandId});
    return probes;
}

void CheckFogDataChoice()
{
    const std::vector<FogDataCandidate> both = {{"fogdata.bin", ProbesOf(kDarkerMidnight)},
                                                {"fogdata-without-darker-nights.bin", ProbesOf(kBrighterMidnight)}};
    std::vector<FogDataScore> scores;
    const int darker = ChooseFogData(both, ReaderOf(kDarkerMidnight), scores);
    const int brighter = ChooseFogData(both, ReaderOf(kBrighterMidnight), scores);
    const FogDataScore brighterScore = scores[1];
    const int unknown = ChooseFogData(both, ReaderOf(0x010203), scores);
    const int unloaded = ChooseFogData(both, [](uint32_t, uint32_t, uint32_t&) { return false; }, scores);
    const std::vector<FogDataCandidate> unprobedDefault = {{"fogdata.bin", {}},
                                                           {"fogdata-without-darker-nights.bin",
                                                            ProbesOf(kBrighterMidnight)}};
    const int fromUnprobed = ChooseFogData(unprobedDefault, ReaderOf(kBrighterMidnight), scores);
    std::printf("     darker nights -> %d, without -> %d (%zu of %zu matched), other table -> %d, not loaded -> %d, "
                "default without probes -> %d\n",
                darker, brighter, brighterScore.matched, brighterScore.read, unknown, unloaded, fromUnprobed);
    Check(darker == 0 && brighter == 1 && brighterScore.matched == 3 && unknown == 0 && unloaded == -1 &&
              fromUnprobed == 1,
          "the fog data whose band probes match the client's LightIntBand is chosen, the default fogdata.bin when "
          "none matches more than half, and nothing before the table is loaded");
}

bool WriteBytes(const std::wstring& path, const std::vector<unsigned char>& bytes)
{
    std::FILE* file = _wfopen(path.c_str(), L"wb");
    if (!file)
        return false;
    const bool written = std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
    return std::fclose(file) == 0 && written;
}

std::vector<unsigned char> WithBandProbes(const std::vector<unsigned char>& formatFour,
                                          const std::vector<FogBandProbe>& probes)
{
    constexpr size_t kVersionOffset = 4;
    constexpr size_t kFormatFourHeaderBytes = 36;
    std::vector<unsigned char> bytes(formatFour.begin(), formatFour.begin() + kFormatFourHeaderBytes);
    const uint32_t version = 5;
    const uint32_t count = static_cast<uint32_t>(probes.size());
    std::memcpy(bytes.data() + kVersionOffset, &version, sizeof(version));
    bytes.insert(bytes.end(), reinterpret_cast<const unsigned char*>(&count),
                 reinterpret_cast<const unsigned char*>(&count) + sizeof(count));
    bytes.insert(bytes.end(), formatFour.begin() + kFormatFourHeaderBytes, formatFour.end());
    bytes.insert(bytes.end(), reinterpret_cast<const unsigned char*>(probes.data()),
                 reinterpret_cast<const unsigned char*>(probes.data() + probes.size()));
    return bytes;
}

void CheckFogDataFilesAreChosen(const std::wstring& outDir, const std::string& dataPath)
{
    std::vector<unsigned char> original;
    if (std::FILE* source = std::fopen(dataPath.c_str(), "rb"))
    {
        unsigned char chunk[65536];
        for (size_t n; (n = std::fread(chunk, 1, sizeof(chunk), source)) > 0;)
            original.insert(original.end(), chunk, chunk + n);
        std::fclose(source);
    }
    uint32_t version = 0;
    if (original.size() >= 8)
        std::memcpy(&version, original.data() + 4, sizeof(version));
    if (version != 4)
    {
        std::printf("     band probe file checks skipped: %s is format %u\n", dataPath.c_str(), version);
        return;
    }
    const std::wstring folder = FullPath(outDir + L"\\fog-data-choice");
    CreateDirectoryW(folder.c_str(), nullptr);
    const std::wstring defaultFile = folder + L"\\fogdata.bin";
    const std::wstring brighterFile = folder + L"\\fogdata-without-darker-nights.bin";
    const bool written = WriteBytes(defaultFile, WithBandProbes(original, ProbesOf(kDarkerMidnight))) &&
                         WriteBytes(brighterFile, WithBandProbes(original, ProbesOf(kBrighterMidnight)));
    FogData data;
    const bool loaded = data.Load(NarrowPath(defaultFile));
    const size_t loadedProbes = data.BandProbes().size();
    std::vector<FogBandProbe> read;
    const bool probesRead = ReadFogBandProbes(NarrowPath(brighterFile), read);

    FogDataChoice choice;
    choice.Prepare(NarrowPath(defaultFile));
    const bool undecidedAtStart = !choice.Decided();
    choice.Decide(data, [](uint32_t, uint32_t, uint32_t&) { return false; });
    const bool waitsForTheTable = !choice.Decided();
    choice.Decide(data, ReaderOf(kBrighterMidnight));
    const bool switched = choice.Decided() && data.Loaded() && data.Path() == NarrowPath(brighterFile);
    std::printf("     format 5 copy: loaded %d with %zu probes, alternate probes %zu (first %u/%u 0x%06X); switched "
                "to %s\n",
                loaded, loadedProbes, read.size(), read.empty() ? 0 : read[0].bandId,
                read.empty() ? 0 : read[0].keyIndex, read.empty() ? 0 : read[0].rgb, data.Path().c_str());
    Check(written && loaded && loadedProbes == 3 && probesRead && read.size() == 3 &&
              read[0].rgb == kBrighterMidnight && undecidedAtStart && waitsForTheTable && switched,
          "format 5 fog data carries its LightIntBand probes, the other fogdata*.bin files of its folder are found, "
          "and once the client's table is loaded the matching one replaces the default");
    DeleteFileW(defaultFile.c_str());
    DeleteFileW(brighterFile.c_str());
    RemoveDirectoryW(folder.c_str());
}
}
