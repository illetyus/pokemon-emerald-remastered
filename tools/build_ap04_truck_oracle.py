"""Unchanged source task functions under semantic host stubs; no GPU/ABI claim."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,re,subprocess
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('init',ROOT/'tools/build_ap04_new_game_init.py');init=importlib.util.module_from_spec(spec);spec.loader.exec_module(init)
FUNCTIONS=['GetTruckCameraBobbingY','GetTruckBoxYMovement','Task_Truck1','Task_Truck2','Task_Truck3','Task_HandleTruckSequence','ExecuteTruckSequence','EndTruckSequence']
def generate(root):
    files=init.load(root)
    path='src/field_special_scene.c'
    source=subprocess.check_output(['git','-C',str(root),'show',init.prepare.SOURCE_TREE+':'+path]).decode()
    c=init.Constants(b.decode() for b in files.values())
    table=re.search(r'static const s8 sTruckCamera_HorizontalTable\[\]\s*=\s*\{[^}]+\};',source).group(0)
    bodies={name:init.prepare.function(source,name) for name in FUNCTIONS}
    text='''#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8;typedef int8_t s8;typedef uint16_t u16;typedef int16_t s16;
typedef void (*TaskFunc)(u8);
struct Task { TaskFunc func;s16 data[16];u8 isActive; };
static struct Task gTasks[16];
static struct { struct { s8 mapGroup,mapNum; } location; } source_save;
static void *source_unused;
static struct { struct { s8 mapGroup,mapNum; } location; } *unused_save;
'''
    # One named type avoids compiler-specific anonymous-struct pointer typing.
    text=text[:text.index('static struct { struct')]+'''struct Save {struct {s8 mapGroup,mapNum;} location;};
static struct Save source_save;
static struct Save *gSaveBlock1Ptr=&source_save;
static struct {u8 active;} gPaletteFade;
static u16 gPlttBufferFaded[512];
static unsigned effects,complete,frames;
static int cameraX,cameraY,boxes[6];
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define MAP_OFFSET 7
#define CpuFastFill(v,p,n) memset((p),(v),(n))
'''
    for name in ['LOCALID_TRUCK_BOX_TOP','LOCALID_TRUCK_BOX_BOTTOM_L','LOCALID_TRUCK_BOX_BOTTOM_R',
        'SE_TRUCK_MOVE','SE_TRUCK_STOP','SE_TRUCK_UNLOAD','SE_TRUCK_DOOR',
        'METATILE_InsideOfTruck_ExitLight_Top','METATILE_InsideOfTruck_ExitLight_Mid','METATILE_InsideOfTruck_ExitLight_Bottom',
        'METATILE_InsideOfTruck_DoorClosedFloor_Top','METATILE_InsideOfTruck_DoorClosedFloor_Mid','METATILE_InsideOfTruck_DoorClosedFloor_Bottom']:
        text+='#define '+name+' '+str(c.value(name))+'\n'
    for name,value in re.findall(r'^#define (BOX[123]_[XY]_OFFSET)\s+(-?\d+)',source,re.M):
        text+='#define '+name+' '+value+'\n'
    text+=table+'\n'
    for body in bodies.values():text+=body[:body.index('{')].strip()+';\n'
    text+='''static void SetObjectEventSpritePosByLocalIdAndMap(u8 id,s8 num,s8 group,s16 x,s16 y) {
    (void)num;(void)group;if(id>=1&&id<=3){boxes[2*(id-1)]=x;boxes[2*(id-1)+1]=y;}
}
static void SetCameraPanning(s16 x,s16 y){cameraX=x;cameraY=y;}
static void SetCameraPanningCallback(void *p){(void)p;}
static void InstallCameraPanAheadCallback(void){effects|=8;}
static u8 CreateTask(TaskFunc func,u8 priority) {
    u8 i;(void)priority;for(i=0;i<16;++i)if(!gTasks[i].isActive){memset(&gTasks[i],0,sizeof gTasks[i]);gTasks[i].func=func;gTasks[i].isActive=1;return i;}return 255;
}
static void DestroyTask(u8 id){gTasks[id].isActive=0;}
static void PlaySE(u16 id){effects|=id==SE_TRUCK_MOVE?1:id==SE_TRUCK_STOP?4:id==SE_TRUCK_UNLOAD?16:32;}
static void FadeInFromBlack(void){effects|=2;}
static void MapGridSetMetatileIdAt(int x,int y,u16 tile){(void)x;(void)y;(void)tile;}
static void DrawWholeMapView(void){}
static void LockPlayerFieldControls(void){}
static void UnlockPlayerFieldControls(void){complete=1;}
static u8 FuncIsActiveTask(TaskFunc func){u8 i;for(i=0;i<16;++i)if(gTasks[i].isActive&&gTasks[i].func==func)return 1;return 0;}
'''
    groups=[(['GetTruckCameraBobbingY','GetTruckBoxYMovement'],'',''),
        (['Task_Truck1'],'#define tTimer data[0]\n','#undef tTimer\n'),
        (['Task_Truck2','Task_Truck3'],'#define tTimerHorizontal data[0]\n#define tMoveStep data[1]\n#define tTimerVertical data[2]\n','#undef tTimerHorizontal\n#undef tMoveStep\n#undef tTimerVertical\n'),
        (['Task_HandleTruckSequence'],'#define tState data[0]\n#define tTimer data[1]\n#define tTaskId1 data[2]\n#define tTaskId2 data[3]\n','#undef tState\n#undef tTimer\n#undef tTaskId1\n#undef tTaskId2\n'),
        (['ExecuteTruckSequence','EndTruckSequence'],'','')]
    for names,before,after in groups:text+=before+'\n'.join(bodies[n] for n in names)+'\n'+after
    text+='''void ap04_source_truck_reset(void) {
    memset(gTasks,0,sizeof gTasks);memset(boxes,0,sizeof boxes);effects=complete=frames=0;cameraX=cameraY=0;
    source_save.location.mapGroup=25;source_save.location.mapNum=40;ExecuteTruckSequence();
}
void ap04_source_truck_tick(unsigned fade,unsigned *outEffects,unsigned *outState,
    unsigned *outTimer,unsigned *outComplete,int output[8]) {
    unsigned i;effects=0;gPaletteFade.active=(u8)fade;++frames;
    for(i=0;i<16;++i)if(gTasks[i].isActive)gTasks[i].func((u8)i);
    *outEffects=effects;*outState=(unsigned)gTasks[0].data[0];*outTimer=(unsigned)gTasks[0].data[1];*outComplete=complete;
    output[0]=cameraX;output[1]=cameraY;memcpy(output+2,boxes,sizeof boxes);
}
'''
    return text,{'SourceCommit':init.prepare.SOURCE_PIN,'SourceFileSHA256':hashlib.sha256(source.encode()).hexdigest(),
        'OriginalFunctions':{n:hashlib.sha256(b.encode()).hexdigest() for n,b in bodies.items()},'GPUVerified':False,'SourceABIVerified':False}
def main():
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    text,contract=generate(args.source);args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(text.encode())
    args.output.with_suffix('.json').write_bytes((json.dumps(contract,indent=2)+'\n').encode())
if __name__=='__main__':main()
