#include "RemasterSessionMenu.h"
#include "RemasterTruckSequence.h"
#include "RemasterRenderProvenance.h"
#include <iostream>
extern "C" {
void ap04_source_truck_reset(void);
void ap04_source_truck_tick(unsigned,unsigned*,unsigned*,unsigned*,unsigned*,int[8]);
}
static unsigned checks,failures;
static void check(bool ok,const char* message){++checks;if(!ok){if(failures<20)std::cerr<<message<<'\n';++failures;}}
int main() {
    using namespace RemasterSession;
    using namespace RemasterRenderProvenance;
    check(IsTruckMap("MAP_INSIDE_OF_TRUCK")&&!IsTruckMap("InsideOfTruck")&&!IsTruckMap("MAP_LITTLEROOT_TOWN"),"truck guard uses canonical catalog identity, not source directory name");
    const std::string sourceCommit="70db90c9077aed1272e746fc2537d9f12b95a91c";
    const std::string payloadHash(64,'a');
    check(GitCommit(sourceCommit),"real source manifest Git SHA1 passes provenance");
    check(!Sha256(sourceCommit),"commit cannot stand in for a payload SHA256");
    check(Sha256(payloadHash)&&!GitCommit(payloadHash),"payload and commit hash domains remain distinct");
    for(const auto& bad:{std::string(),sourceCommit.substr(1),sourceCommit+"0",std::string(40,'g'),std::string(40,' ')})
        check(!GitCommit(bad),"malformed source commit is rejected");
    for(const auto& bad:{std::string(),payloadHash.substr(1),payloadHash+"0",std::string(64,'g'),std::string(64,' ')})
        check(!Sha256(bad),"malformed content hash is rejected");
    const auto horizontal=SourcePixelLocal(3,0,6.25);
    const auto vertical=SourcePixelLocal(0,3,6.25);
    check(horizontal.x==0&&horizontal.y==18.75&&horizontal.z==0,"horizontal source pan uses camera right, never forward");
    check(vertical.x==0&&vertical.y==0&&vertical.z==-18.75,"positive source screen Y uses camera down");
    Menu menu;menu.Build();check(!menu.frame.rows[0].enabled&&menu.frame.rows[1].enabled,"missing file offers New Game only");
    auto revision=menu.frame.revision;menu.Activate(1,revision);
    check(menu.screen==MenuScreen::NewGame,"source identity form opens");
    check(menu.Activate(2,revision)==MenuCommand::None,"old title click cannot submit new form");
    menu.SetName("ABCDEFGH");check(!menu.frame.rows[2].enabled,"oversize name cannot begin");
    menu.SetName("Cemal");menu.Activate(1,menu.frame.revision);check(menu.avatar==1&&menu.name=="Cemal","avatar changes preserve custom name");
    check(menu.Activate(2,menu.frame.revision)==MenuCommand::Start&&menu.screen==MenuScreen::Busy,"validated form emits one start");
    check(menu.Activate(2,menu.frame.revision)==MenuCommand::None,"busy frame cannot duplicate start");
    for(auto status:{REMASTER_EMERALD_SAVE_OK,REMASTER_EMERALD_SAVE_DEGRADED,REMASTER_EMERALD_SAVE_CORRUPT,REMASTER_EMERALD_SAVE_IO_ERROR,REMASTER_EMERALD_SAVE_UNSUPPORTED}) {
        menu.status=status;menu.Show(MenuScreen::Title);
        check(menu.frame.rows[0].enabled==CanContinue(status)&&!menu.frame.rows[1].enabled,"existing/error provenance never permits new overwrite");
    }
    menu.Activate(2,menu.frame.revision);menu.SetPath("D:/private/source.sav");
    check(!menu.frame.rows[2].enabled,"import requires explicit source format");
    menu.Activate(0,menu.frame.revision);check(menu.frame.rows[2].enabled,"explicit source format enables copy-first import");
    FrameFence fence;check(fence.Advance(),"first publication");const auto title=fence.revision;
    check(fence.Advance()&&!fence.Matches(static_cast<std::int64_t>(title)),"frontend revision cannot become a gameplay click");
    fence.revision=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    check(!fence.Advance()&&!fence.Matches(static_cast<std::int64_t>(fence.revision)),"exhausted publication fence rejects all clicks");
    Lifecycle lifecycle;check(lifecycle.Begin(),"begin source intro");auto token=lifecycle.generation;
    check(lifecycle.FinishIntro(token,REMASTER_EMERALD_SAVE_OK,true)&&!lifecycle.Playing(),"decoded map readiness is not completed intro");
    check(!lifecycle.Begin()&&!lifecycle.CompleteIntro(token-1,true),"duplicate/stale intro rejected");
    check(lifecycle.CompleteIntro(token,true)&&lifecycle.Playing()&&!lifecycle.CompleteIntro(token,true),"real current intro completion exactly once");
    for(unsigned delay:{0u,600u,1200u}) {
        TruckSequence truck;ap04_source_truck_reset();
        for(unsigned frame=1;frame<2000&&!truck.complete;++frame) {
            const unsigned fading=frame<delay?1:0;
            const auto effects=truck.Tick(fading!=0);
            unsigned originalEffects,state,timer,complete;int values[8];
            ap04_source_truck_tick(fading,&originalEffects,&state,&timer,&complete,values);
            check(effects==originalEffects,"source truck effects and ordering");
            check(truck.state==state&&truck.timer==timer&&truck.complete==(complete!=0),"source truck state/waits");
            check(truck.cameraX==values[0]&&truck.cameraY==values[1],"source camera bob/stop sequence");
            for(unsigned i=0;i<3;++i)check(truck.boxX[i]==values[2+2*i]&&truck.boxY[i]==values[3+2*i],"source box offsets");
        }
        check(truck.complete,"source sequence finishes after actual fade completion");
        check(truck.Tick()==TruckNone,"completed sequence emits no duplicate door/unlock");
    }
    std::cout<<"AP04 frontend/truck: "<<checks<<" checks, "<<failures<<" failures\n";return failures?1:0;
}
