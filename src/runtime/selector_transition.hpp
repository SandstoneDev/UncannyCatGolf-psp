#pragma once

#include "asset_file.hpp"
#include "level_select.hpp"
#include "selector_transition_compact.hpp"

#include <algorithm>
#include <pspkernel.h>
#include <cstring>
#include <vector>

class SelectorTransitionArt {
public:
    static constexpr unsigned Frames = SelectorTransitionCompact::Frames;
    static constexpr unsigned Fps = SelectorTransitionCompact::Fps;
    static constexpr unsigned DurationMs = Frames * 1000 / Fps;

    bool open(const char* filename) {
        closeStream();
        path.clear();
        const auto input = openAssetFile(filename);
        if (!codec.open(input.get())) return false;
        path.assign(filename, filename + std::strlen(filename) + 1);
        return true;
    }

    void closeStream() { stream.reset(); }

    const uint16_t* frame(unsigned from, unsigned to, unsigned latest, bool russian, unsigned frameNumber) {
        if (latest >= Worlds || from > latest || to > latest || frameNumber >= Frames ||
            std::max(from, to) - std::min(from, to) != 1) return nullptr;
        const unsigned clip = SelectorTransitionCompact::ClipsPerLanguage * static_cast<unsigned>(russian) +
                              latest * (latest - 1) + 2 * std::min(from, to) +
                              static_cast<unsigned>(from > to);
        const unsigned id = clip * Frames + frameNumber;
        if (const auto* cached = codec.cached(id)) return cached;
        if (path.empty()) return nullptr;
        if (!stream) stream = openAssetFile(path.data());
        const auto* pixels = codec.frame(stream.get(), id);
        if (!pixels) {
            closeStream();
            return nullptr;
        }
        sceKernelDcacheWritebackRange(pixels, SelectorTransitionCompact::FrameBytes);
        return pixels;
    }

private:
    static constexpr unsigned Worlds = LevelSelectState::WorldCount;
    static_assert(Worlds == SelectorTransitionCompact::Worlds);
    std::vector<char> path;
    AssetFile stream;
    SelectorTransitionCompact codec;
};
