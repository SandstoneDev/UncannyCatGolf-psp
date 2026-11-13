#pragma once

#include "asset_file.hpp"
#include "slot_machine.hpp"

#include <cstddef>
#include <cstdlib>
#include <malloc.h>
#include <new>
#include <pspkernel.h>
#include <string>

struct SlotMachineArt {
    static constexpr unsigned ReelWidth = 512, ReelHeight = 256;
    static constexpr unsigned EyeSize = 128;
    static constexpr unsigned OverlaySize = 16;
    static constexpr unsigned TextWidth = 256, TextHeight = 32;

    bool open(std::string name) {
        const auto file = openAssetFile(name.c_str());
        unsigned header[2]{};
        if (!file || std::fread(header, sizeof header, 1, file.get()) != 1 ||
            header[0] != 0x4c534355 || header[1] != OutcomeCount ||
            std::fseek(file.get(), 0, SEEK_END) != 0 ||
            std::ftell(file.get()) != static_cast<long>(PagesOffset + 2 * OutcomeCount * sizeof(Cache::text))) return false;
        path = std::move(name);
        return true;
    }

    bool load() {
        if (cache) return true;
        const auto file = openAssetFile(path.c_str());
        if (!file || std::fseek(file.get(), 2 * sizeof(uint32_t), SEEK_SET) != 0) return false;
        auto* const storage = memalign(64, sizeof(Cache));
        if (!storage) return false;
        std::unique_ptr<Cache,FreeCache> memory(new (storage) Cache);
        if (std::fread(memory.get(), PagesOffset - 2 * sizeof(uint32_t), 1, file.get()) != 1)
            return false;
        sceKernelDcacheWritebackRange(memory.get(), PagesOffset - 2 * sizeof(uint32_t));
        cache = std::move(memory);
        return true;
    }

    void unload() { cache.reset(); cached = NoPage; }
    const uint16_t* reels() const { return cache->reels; }
    const uint16_t* eyes() const { return cache->eyes; }
    const uint32_t* overlay(unsigned frame) const { return cache->overlays[frame]; }

    const uint16_t* marquee(SlotOutcome outcome, bool russian) {
        const unsigned page = russian * OutcomeCount + static_cast<unsigned>(outcome);
        auto& text = cache->text;
        if (page != cached) {
            const auto file = openAssetFile(path.c_str());
            cached = NoPage;
            if (!file || std::fseek(file.get(), PagesOffset + page * sizeof text, SEEK_SET) != 0 ||
                std::fread(text, sizeof text, 1, file.get()) != 1) return nullptr;
            sceKernelDcacheWritebackRange(text, sizeof text);
            cached = page;
        }
        return text;
    }

private:
    static constexpr unsigned OutcomeCount = static_cast<unsigned>(SlotOutcome::Count);
    struct alignas(64) Cache {
        uint16_t reels[ReelWidth * ReelHeight], eyes[EyeSize * EyeSize];
        uint32_t overlays[2][OverlaySize * OverlaySize];
        uint16_t text[TextWidth * TextHeight];
    };
    struct FreeCache { void operator()(Cache* value) const { std::free(value); } };
    static constexpr unsigned PagesOffset = 8 + offsetof(Cache,text);
    static constexpr unsigned NoPage = 2 * OutcomeCount;
    std::unique_ptr<Cache,FreeCache> cache;
    std::string path;
    unsigned cached = NoPage;
};
