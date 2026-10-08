#include "RemasterInputSubsystem.h"
#include "Containers/Ticker.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"

void URemasterInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    BackgroundHandle=FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this,&URemasterInputSubsystem::Background);
    ForegroundHandle=FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this,&URemasterInputSubsystem::Foreground);
    InactiveHandle=FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this,&URemasterInputSubsystem::Inactive);
    ReactivatedHandle=FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this,&URemasterInputSubsystem::Reactivated);
    DeviceHandle=IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddUObject(this,&URemasterInputSubsystem::DeviceConnection);
    TickHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&URemasterInputSubsystem::ObserveLifecycle),1.f/60.f);
}
void URemasterInputSubsystem::LifecycleFence()
{
    RemasterControls::FenceSources(Common,Touch,Digital,EnhancedAxis,GamepadAxis);
    if (auto* PC=GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr)
    {
        PC->FlushPressedKeys();
        if (auto* Local=PC->GetLocalPlayer())
            if (auto* Enhanced=Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                FModifyContextOptions Options;Options.bIgnoreAllPressedKeysUntilRelease=true;
                Enhanced->RequestRebuildControlMappings(Options,EInputMappingRebuildType::RebuildWithFlush);
            }
    }
}
void URemasterInputSubsystem::SetSuspended(RemasterControls::Suspension Reason,bool bSuspended)
{
    if (!Lifecycle.Set(Reason,bSuspended)) return;
    LifecycleFence();RefreshContext();
}
void URemasterInputSubsystem::Background(){SetSuspended(RemasterControls::Suspension::Background,true);}
void URemasterInputSubsystem::Foreground(){SetSuspended(RemasterControls::Suspension::Background,false);}
void URemasterInputSubsystem::Inactive(){SetSuspended(RemasterControls::Suspension::Inactive,true);}
void URemasterInputSubsystem::Reactivated(){SetSuspended(RemasterControls::Suspension::Inactive,false);}
void URemasterInputSubsystem::DeviceConnection(EInputDeviceConnectionState,FPlatformUserId,FInputDeviceId)
{
    // Any controller connection transition conservatively fences every input source.
    LifecycleFence();RefreshContext();
}
bool URemasterInputSubsystem::ObserveLifecycle(float)
{
    RefreshContext(); // Core ticker observes pause even when the world stops ticking.
    return true;
}

void URemasterInputSubsystem::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
    FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(InactiveHandle);
    FCoreDelegates::ApplicationHasReactivatedDelegate.Remove(ReactivatedHandle);
    IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().Remove(DeviceHandle);
    LifecycleFence();
    ResetInputs();
    Digital.Reset();
    EnhancedAxis.Reset();
    GamepadAxis.Reset();
    OnDispatch.Unbind();
    OnReadContext.Unbind();
    FieldOwner.Unbind();
    BattleOwner.Unbind();
    Super::Deinitialize();
}
bool URemasterInputSubsystem::AttachRouting(UObject* Owner,const FRemasterInputContextReader& Reader,
    const FRemasterInputDispatch& Dispatch)
{
    if (!Owner || !Reader.IsBoundToObject(Owner) || !Dispatch.IsBoundToObject(Owner)
        || (OnDispatch.IsBound() && !OnDispatch.IsBoundToObject(Owner))) return false;
    ResetInputs();
    OnReadContext=Reader;OnDispatch=Dispatch;
    RefreshContext();
    return true;
}
void URemasterInputSubsystem::DetachRouting(const UObject* Owner)
{
    if (!Owner || !OnDispatch.IsBoundToObject(Owner)) return;
    LifecycleFence();OnReadContext.Unbind();OnDispatch.Unbind();
    SetContext({});
}
void URemasterInputSubsystem::SetFieldOwner(const FRemasterInputActionOwner& Owner)
{
    ResetInputs();FieldOwner=Owner;RefreshContext();
}
void URemasterInputSubsystem::SetBattleOwner(const FRemasterInputActionOwner& Owner)
{
    ResetInputs();BattleOwner=Owner;RefreshContext();
}
bool URemasterInputSubsystem::DispatchNativeOwner(RemasterControls::Action Action,RemasterControls::Target Target)
{
    if (Target==RemasterControls::Target::FieldInteract)
        return FieldOwner.IsBound() && FieldOwner.Execute(Action);
    if (Target==RemasterControls::Target::BattleInput)
        return BattleOwner.IsBound() && BattleOwner.Execute(Action);
    return false;
}
void URemasterInputSubsystem::RefreshContext()
{
    if (Lifecycle.Set(RemasterControls::Suspension::Paused,
        GetWorld() && UGameplayStatics::IsGamePaused(GetWorld()))) LifecycleFence();
    auto Context=OnReadContext.IsBound() ? OnReadContext.Execute() : RemasterControls::Context{};
    Context.suspensionReasons|=Lifecycle.Reasons();
    Context.fieldAttached=FieldOwner.IsBound();Context.battleAttached=BattleOwner.IsBound();
    SetContext(Context);
}
void URemasterInputSubsystem::SetContext(RemasterControls::Context Context)
{
    // An unattached delivery endpoint is never advertised as a usable owner.
    if (!OnDispatch.IsBound()) Context={};
    const auto Before=Common.Epoch();
    Common.SetContext(Context);
    if (Common.Epoch()!=Before || Common.CurrentFocus()==RemasterControls::Focus::Blocked)
        Touch.Reset();
}
void URemasterInputSubsystem::ResetInputs()
{
    Common.Reset();
    Touch.Reset();
}
bool URemasterInputSubsystem::SetTouchLayout(const RemasterControls::Layout& Layout)
{
    if (!Touch.SetLayout(Layout)) return false;
    ResetInputs();
    return true;
}
RemasterControls::Status URemasterInputSubsystem::SubmitEvent(const RemasterControls::Event& Event)
{
    RefreshContext();
    const auto Result=Common.Submit(Event);
    if (Result.status!=RemasterControls::Status::Dispatch) return Result.status;
    if (!OnDispatch.IsBound() || !OnDispatch.Execute(Result.action,Result.target))
    {
        ResetInputs();
        return RemasterControls::Status::Unsupported;
    }
    RefreshContext();
    return Result.status;
}
RemasterControls::Status URemasterInputSubsystem::SubmitPhysical(RemasterControls::Source Source,
    uint16 Control,RemasterControls::Action Action,RemasterControls::Phase Phase)
{
    RefreshContext();
    const auto Packet=Digital.Process(Source,0,Control,Action,Phase,Common.Epoch());
    return Packet.hasEvent ? SubmitEvent(Packet.event) : Packet.status;
}
RemasterControls::Status URemasterInputSubsystem::SubmitAxis(FVector2D Axis,bool bGamepad)
{
    RefreshContext();
    auto& Adapter=bGamepad ? GamepadAxis : EnhancedAxis;
    const auto Batch=Adapter.Process(Axis.X,Axis.Y,Common.Epoch());
    auto Status=Batch.status;
    for(unsigned i=0;i<Batch.count;++i) Status=SubmitEvent(Batch.events[i]);
    return Batch.status==RemasterControls::Status::Invalid ? Batch.status : Status;
}
RemasterControls::Status URemasterInputSubsystem::SubmitTouch(
    const RemasterControls::TouchPacket& Packet)
{
    if (!Packet.valid) return RemasterControls::Status::Invalid;
    if (!Packet.hasEvent) return RemasterControls::Status::Ignored;
    const auto Result=SubmitEvent(Packet.event);
    if (Result==RemasterControls::Status::Blocked || Result==RemasterControls::Status::Unsupported
        || Result==RemasterControls::Status::Full || Result==RemasterControls::Status::Exhausted)
        Touch.Cancel(Packet.event.control,Common.Epoch());
    return Result;
}
RemasterControls::Status URemasterInputSubsystem::TouchPressed(int32 Finger,FVector2D Normalized)
{
    if (Finger<0) return RemasterControls::Status::Invalid;
    RefreshContext();
    return SubmitTouch(Touch.Press(static_cast<unsigned>(Finger),Normalized.X,Normalized.Y,Common.Epoch()));
}
RemasterControls::Status URemasterInputSubsystem::TouchMoved(int32 Finger,FVector2D Normalized)
{
    if (Finger<0) return RemasterControls::Status::Invalid;
    RefreshContext();
    return SubmitTouch(Touch.Move(static_cast<unsigned>(Finger),Normalized.X,Normalized.Y,Common.Epoch()));
}
RemasterControls::Status URemasterInputSubsystem::TouchReleased(int32 Finger)
{
    if (Finger<0) return RemasterControls::Status::Invalid;
    RefreshContext();
    return SubmitTouch(Touch.Release(static_cast<unsigned>(Finger),Common.Epoch()));
}

