#pragma once

#include "cat_artist_palette.hpp"
#include "cat_artist_global.hpp"

namespace ArtistCopycat {

std::string root,prefix;
std::string diagnostics;
unsigned preferred=ArtistGlobal::NoProfile;
bool loaded=false;
unsigned base=0;
bool drawn=false;
alignas(64) ArtistBook::Canvas pixels{};

void update(const ArtistBook::Book& book) {
    loaded=true;
    base=book.bases[book.active];
    const auto& canvas=book.art[book.active];
    drawn=std::any_of(canvas.begin(),canvas.end(),[](uint8_t value) { return value!=0; });
    if (!drawn) return;
    ArtistInk::swizzle(canvas,pixels.data());
    sceKernelDcacheWritebackRange(pixels.data(),pixels.size());
}

bool loadBook(ArtistBook::Book& book) {
    ArtistGlobal::Choice choice;
    const auto status=ArtistGlobal::load(root,preferred,book,choice);
    if ((!diagnostics.empty()) && (choice.sourceGeneration || status!=ArtistGlobal::Status::Valid)) {
        if (auto* file=std::fopen(diagnostics.c_str(),"a")) {
            std::fprintf(file,"event=cat_artist_global status=%u source_profile=%u generation=%u checksum=%08x inspected_valid_mask=%02x\n",
                         unsigned(status),choice.profile,unsigned(choice.sourceGeneration),unsigned(choice.sourceChecksum),choice.validProfiles);
            std::fclose(file);
        }
    }
    return status==ArtistGlobal::Status::Valid;
}

void setProfile(unsigned profile) {
    preferred=profile;
    const auto next=ArtistGlobal::shared(root);
    if (prefix==next && loaded) return;
    prefix=next;loaded=false;
    std::unique_ptr<ArtistBook::Book> book(new(std::nothrow) ArtistBook::Book);
    if (!book || !loadBook(*book)) {base=0;drawn=false;return;}
    update(*book);
}

VillaCostumes::Pair costume() {
    using VillaCostumes::Id;
    if (base==ArtistBook::Blank)
        return {VillaCostumes::Art::BlankArt,VillaCostumes::row(Id::Canny).ghost};
    return VillaCostumes::actorPair(static_cast<Id>(base));
}

} // namespace ArtistCopycat
