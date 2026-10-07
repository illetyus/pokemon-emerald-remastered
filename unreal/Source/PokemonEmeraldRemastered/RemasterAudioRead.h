#pragma once
// Portable presentation-only resolver. No mutable core pointer or audio paths.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
extern "C" {
#include "remaster/emerald_script_host.h"
}
namespace RemasterAudio {
enum class Category { Silence, Music, Sfx, Jingle, Phoneme, Cry, Ambience };
struct SongEntry { unsigned id; Category category; const char* identity; };
struct CryEntry { unsigned coreSpecies, nationalDex, sourceIndex; const char* identity; };
struct FanfareEntry { unsigned id, song, sourceWaitFrames; };
#include "RemasterAudioCatalog.inl"
struct Request { std::string identity; Category category = Category::Silence; bool stopMusic = false; };
inline bool Song(unsigned id, Request& out) {
    if (id == 65535 || id == 0) { out = {"song.0", Category::Silence, true}; return true; }
    if (id >= sizeof(Songs)/sizeof(Songs[0])) return false;
    out = {Songs[id].identity, Songs[id].category, false}; return true;
}
inline bool Fanfare(unsigned id, Request& out) {
    for (const auto& entry : Fanfares) if (entry.id == id) return Song(entry.song, out);
    return false;
}
inline bool Cry(unsigned species, unsigned mode, Request& out) {
    if (mode > 12) return false;
    if (species >= 413 && species <= 439) species = 201;
    for (const auto& entry : Cries) if (entry.coreSpecies == species) {
        out = {std::string(entry.identity)+".mode."+std::to_string(mode), Category::Cry, false}; return true;
    }
    return false;
}
enum class ScriptRead { Ready, HostOwnedWait, NotAudio, Unsupported };
inline ScriptRead Script(const RemasterEmeraldScriptRequest& source, Request& out) {
    bool audio=source.type==REMASTER_EMERALD_SCRIPT_REQUEST_SOUND ||
        source.type==REMASTER_EMERALD_SCRIPT_REQUEST_FANFARE || source.type==REMASTER_EMERALD_SCRIPT_REQUEST_BGM;
    if (!audio) return ScriptRead::NotAudio;
    if (source.action==REMASTER_EMERALD_SCRIPT_ASYNC_WAIT && source.type!=REMASTER_EMERALD_SCRIPT_REQUEST_BGM) return ScriptRead::HostOwnedWait;
    if (source.action!=REMASTER_EMERALD_SCRIPT_ASYNC_START) return ScriptRead::Unsupported;
    // Runtime copies instruction .a to local_id; value_u16 is .b (BGM save flag).
    // Script PLAY_FANFARE takes a song ID, not a FANFARE_* table index.
    bool valid=Song(source.local_id,out);
    if (valid && source.type==REMASTER_EMERALD_SCRIPT_REQUEST_FANFARE && out.category!=Category::Jingle) valid=false;
    if (valid && source.type==REMASTER_EMERALD_SCRIPT_REQUEST_BGM && !out.stopMusic) out.category=Category::Music;
    return valid?ScriptRead::Ready:ScriptRead::Unsupported;
}
struct Settings {
    float master=1, music=.8f, sfx=.8f, ambience=.6f;
    static float Unit(float v,float fallback) { return std::isfinite(v)?std::clamp(v,0.f,1.f):fallback; }
    void Sanitize() { master=Unit(master,1);music=Unit(music,.8f);sfx=Unit(sfx,.8f);ambience=Unit(ambience,.6f); }
    float Gain(Category c) const { return master*(c==Category::Music?music:c==Category::Ambience?ambience:sfx); }
};
struct Voice { uint64_t id; unsigned priority; };
struct Admission { bool accepted=false; uint64_t evicted=0; };
class Policy {
    std::vector<Voice> voices;
    bool background=false, focusLost=false;
public:
    static constexpr unsigned Limit=16;
    bool Suspended() const { return background||focusLost; }
    void Background(bool value) { background=value; }
    void Focus(bool present) { focusLost=!present; }
    void Finished(uint64_t id) { voices.erase(std::remove_if(voices.begin(),voices.end(),[id](const Voice& v){return v.id==id;}),voices.end()); }
    void Reset() { voices.clear(); }
    size_t Active() const { return voices.size(); }
    Admission Admit(uint64_t id,unsigned priority) {
        if (!id||Suspended()||std::any_of(voices.begin(),voices.end(),[id](const Voice& v){return v.id==id;})) return {};
        uint64_t evicted=0;
        if (voices.size()>=Limit) {
            const auto low=std::min_element(voices.begin(),voices.end(),[](const Voice& a,const Voice& b){return a.priority<b.priority;});
            if (low->priority>=priority) return {};
            evicted=low->id;voices.erase(low);
        }
        voices.push_back({id,priority});return {true,evicted};
    }
};
}
