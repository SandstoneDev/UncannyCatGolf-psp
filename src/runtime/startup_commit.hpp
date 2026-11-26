#pragma once
#include "startup_intro_state.hpp"
#include "startup_editor_bus.hpp"

namespace StartupCommit {
struct Boot { StartupSettings::Values values{}; bool needed=false; };
inline Boot inspect(const StartupSettings::File& file,const std::string& languagePath,
                    const std::string& editorPath,const std::string& backgroundPath,bool returning) {
    const auto stored=file.read();
    Boot result;result.values=stored.values;
    auto legacy=result.values;
    const bool languageValid=StartupSettings::legacyLanguage(languagePath,legacy);
    if (languageValid && stored.status!=StartupSettings::Status::Pending)
        result.values.language=legacy.language;
    legacy=result.values;
    const bool editorValid=StartupEditorBus::load(editorPath,legacy.master,legacy.music,legacy.sounds)==StartupEditorBus::Load::Valid;
    if (editorValid && stored.status!=StartupSettings::Status::Pending) {
        result.values.master=legacy.master;result.values.music=legacy.music;result.values.sounds=legacy.sounds;
    }
    const bool backgroundValid=StartupSettings::legacyBackground(backgroundPath);
    result.needed=StartupSettings::showIntro(stored.status,languageValid||editorValid||backgroundValid,returning);
    return result;
}

template<class Reload,class SaveLanguage>
bool complete(const StartupSettings::File& file,const std::string& editorPath,
              const StartupSettings::Values& values,Reload reload,SaveLanguage saveLanguage,
              StartupIntroState::Error& error) {
    using Error=StartupIntroState::Error;
    error=Error::Save;
    // A checked Pending record precedes any legacy compatibility write. A failed
    // two-file transaction must never masquerade as a completed legacy install.
    if (!file.stage(values)) return false;
    unsigned master=20,music=20,sounds=20;
    const auto status=StartupEditorBus::load(editorPath,master,music,sounds);
    std::string message;
    const bool shared=status==StartupEditorBus::Load::Invalid ?
        StartupEditorBus::resetInvalid(editorPath,values.master,values.music,values.sounds,message):
        StartupEditorBus::merge(editorPath,values.master,values.music,values.sounds,message);
    if (!shared) return false;
    if (!reload(values.language)) {error=Error::Language;return false;}
    if (!saveLanguage(values.language) || !file.commit(values)) return false;
    error=Error::None;return true;
}
}
