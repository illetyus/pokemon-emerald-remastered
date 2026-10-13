#pragma once
#include <array>
#include <cstdint>

namespace RemasterSession {
enum TruckEffect : unsigned { TruckNone=0,TruckMove=1,TruckFadeIn=2,TruckStop=4,TruckCameraRestore=8,TruckUnload=16,TruckDoor=32 };
// Host cutscene lifetime, ported from pinned field_special_scene.c. It never
// advances player coordinates/flags or acknowledges a script VM request.
struct TruckSequence {
    unsigned state=0,timer=0,child=0,horizontal=0,vertical=0,step=0,frames=0;
    int cameraX=0,cameraY=0;
    std::array<int,3> boxX{{0,0,0}},boxY{{0,0,0}};
    bool complete=false;
    static int Bob(int time) { return time%120==0?-1:time%10<=4?1:0; }
    static int Box(int time) { return (time+120)%180==0?-1:0; }
    unsigned Tick(bool fadeActive=false) {
        if(complete)return TruckNone;
        ++frames;unsigned effects=TruckNone;
        switch(state) {
        case 0:if(++timer==90){timer=0;child=1;state=1;effects|=TruckMove;}break;
        case 1:if(++timer==150){timer=0;state=2;effects|=TruckFadeIn;}break;
        case 2:if(++timer>300&&!fadeActive){timer=0;child=2;horizontal=vertical=step=0;state=3;effects|=TruckStop;}break;
        case 3:if(!child){timer=0;state=4;effects|=TruckCameraRestore;}break;
        case 4:if(++timer==90){timer=0;state=5;effects|=TruckUnload;}break;
        case 5:if(++timer==120){child=0;complete=true;effects|=TruckDoor;}break;
        default:break;
        }
        if(child==1) {
            boxX={{3,0,-3}};boxY={{3+Box(static_cast<int>(vertical)+30)*4,-3+Box(static_cast<int>(vertical))*2,Box(static_cast<int>(vertical))*4}};
            if(++vertical==30000)vertical=0;
            cameraX=0;cameraY=Bob(static_cast<int>(vertical));
        } else if(child==2 || child==3) {
            static constexpr int table[19]={0,0,0,0,0,0,0,0,1,2,2,2,2,2,2,-1,-1,-1,0};
            const unsigned oldChild=child;
            ++horizontal;if(oldChild==2)++vertical;
            if(horizontal>5){horizontal=0;++step;}
            if(step==19)child=0;
            else {
                if(oldChild==2&&table[step]==2)child=3;
                cameraX=table[step];cameraY=oldChild==2?Bob(static_cast<int>(vertical)):0;
                boxX={{3-cameraX,-cameraX,-3-cameraX}};
                boxY=oldChild==2?std::array<int,3>{{3+Box(static_cast<int>(vertical)+30)*4,-3+Box(static_cast<int>(vertical))*2,Box(static_cast<int>(vertical))*4}}:std::array<int,3>{{3,-3,0}};
            }
        }
        return effects;
    }
};
}
