#pragma once
#include "RemasterSessionModel.h"
extern "C" {
#include "remaster/emerald_new_game.h"
}
#include <string>
#include <vector>

namespace RemasterSession {
enum class MenuScreen { Title, NewGame, Import, Busy, Intro, Error };
enum class MenuCommand { None, Continue, NewGame, Import, Male, Female, Start, VanillaPlus, Stock, ImportCopy, Back };
struct MenuRow { std::string label;MenuCommand command;bool enabled; };
struct MenuFrame {
    std::string title,body,name,path;
    std::vector<MenuRow> rows;
    MenuScreen screen=MenuScreen::Title;
    unsigned selected=0;
    std::uint64_t revision=1;
};
// One publication fence spans frontend and gameplay. Model-local revisions are
// not interchangeable when a late UMG click crosses the session boundary.
struct FrameFence {
    std::uint64_t revision=0;
    bool exhausted=false;
    bool Advance() {
        if(exhausted || revision==static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())){exhausted=true;return false;}
        ++revision;return true;
    }
    bool Matches(std::int64_t expected) const { return !exhausted && expected>0 && static_cast<std::uint64_t>(expected)==revision; }
};
class Menu {
public:
    MenuFrame frame;
    RemasterEmeraldSaveStatus status=REMASTER_EMERALD_SAVE_EMPTY;
    std::string name="BRENDAN",path,notice;
    std::uint8_t avatar=0;
    RemasterEmeraldSaveFormat format=REMASTER_EMERALD_SAVE_FORMAT_UNSPECIFIED;
    MenuScreen screen=MenuScreen::Title;
    void Changed() { if(frame.revision!=std::numeric_limits<std::uint64_t>::max())++frame.revision; }
    void Show(MenuScreen next) { screen=next;frame.selected=0;Changed();Build(); }
    bool SetName(const std::string& value) {
        if(screen!=MenuScreen::NewGame || value.size()>128)return false;
        if(name!=value){name=value;Changed();}Build();return true;
    }
    bool SetPath(const std::string& value) {
        if(screen!=MenuScreen::Import || value.size()>4096)return false;
        if(path!=value){path=value;Changed();}Build();return true;
    }
    void Navigate(bool up) {
        if(frame.rows.empty())return;
        const auto count=static_cast<unsigned>(frame.rows.size());
        frame.selected=(frame.selected+(up?count-1:1))%count;Changed();Build();
    }
    bool ValidName() const {
        std::uint8_t encoded[8];
        return remaster_emerald_new_game_encode_name(name.data(),name.size(),encoded)==REMASTER_EMERALD_NEW_GAME_PREPARED;
    }
    void Build() {
        frame.screen=screen;frame.name=name;frame.path=path;frame.rows.clear();
        frame.title="Pokémon Emerald Remastered";frame.body=notice;
        auto add=[this](const char* label,MenuCommand command,bool enabled=true){frame.rows.push_back({label,command,enabled});};
        if(screen==MenuScreen::Title || screen==MenuScreen::Error) {
            if(notice.empty()) {
                switch(status) {
                case REMASTER_EMERALD_SAVE_EMPTY:frame.body="Yeni bir yolculuğa başlayabilirsin.";break;
                case REMASTER_EMERALD_SAVE_OK:frame.body="Yolculuğuna kaldığın yerden devam et.";break;
                case REMASTER_EMERALD_SAVE_DEGRADED:frame.body="Kurtarılabilen kayıtla devam edebilirsin.";break;
                case REMASTER_EMERALD_SAVE_CORRUPT:frame.body="Kayıt bozuk. Başka bir kayıt aktarabilirsin.";break;
                case REMASTER_EMERALD_SAVE_UNSUPPORTED:frame.body="Kayıt biçimi desteklenmiyor.";break;
                case REMASTER_EMERALD_SAVE_IO_ERROR:
                default:frame.body="Kayıt okunamadı. Tekrar deneyebilir veya başka bir kayıt aktarabilirsin.";break;
                }
            }
            add("Devam Et",MenuCommand::Continue,CanContinue(status));
            add("Yeni Oyun",MenuCommand::NewGame,CanNewGame(status));
            add("Kayıt Aktar",MenuCommand::Import);
        } else if(screen==MenuScreen::NewGame) {
            frame.title="Yeni Oyun";
            if(notice.empty())frame.body="Adını yaz, karakterini seç ve yolculuğa başla.";
            add(avatar==0?"Brendan ✓":"Brendan",MenuCommand::Male);
            add(avatar==1?"May ✓":"May",MenuCommand::Female);
            add("Yolculuğa Başla",MenuCommand::Start,CanNewGame(status)&&ValidName());
            add("Geri",MenuCommand::Back);
        } else if(screen==MenuScreen::Import) {
            frame.title="Kayıt Aktar";
            if(notice.empty())frame.body="Kayıt dosyasının yolunu yaz ve kaynak biçimini seç. Dosyanın bir kopyası kullanılacak.";
            add(format==REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS?"Vanilla+ ✓":"Vanilla+",MenuCommand::VanillaPlus);
            add(format==REMASTER_EMERALD_SAVE_FORMAT_STOCK?"Emerald ✓":"Emerald",MenuCommand::Stock);
            add("Kopyayı Aktar",MenuCommand::ImportCopy,!path.empty()&&format!=REMASTER_EMERALD_SAVE_FORMAT_UNSPECIFIED);
            add("Geri",MenuCommand::Back);
        } else if(screen==MenuScreen::Busy)frame.body="Yükleniyor…";
        else { frame.title="Littleroot'a Yolculuk";frame.body="Taşınma kamyonu…"; }
        if(frame.selected>=frame.rows.size())frame.selected=0;
    }
    MenuCommand Activate(unsigned row,std::uint64_t revision) {
        if(revision!=frame.revision || row>=frame.rows.size() || !frame.rows[row].enabled)return MenuCommand::None;
        const auto command=frame.rows[row].command;
        notice.clear();Changed();
        switch(command) {
        case MenuCommand::NewGame:Show(MenuScreen::NewGame);return MenuCommand::None;
        case MenuCommand::Import:format=REMASTER_EMERALD_SAVE_FORMAT_UNSPECIFIED;Show(MenuScreen::Import);return MenuCommand::None;
        case MenuCommand::Male:case MenuCommand::Female:
            avatar=command==MenuCommand::Male?0:1;
            if(name=="BRENDAN" || name=="MAY")name=avatar==0?"BRENDAN":"MAY";
            Build();return MenuCommand::None;
        case MenuCommand::VanillaPlus:format=REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS;Build();return MenuCommand::None;
        case MenuCommand::Stock:format=REMASTER_EMERALD_SAVE_FORMAT_STOCK;Build();return MenuCommand::None;
        case MenuCommand::Back:Show(MenuScreen::Title);return MenuCommand::None;
        default:Show(MenuScreen::Busy);return command;
        }
    }
};
}
