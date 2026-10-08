#include "RemasterInputSubsystem.h"

void URemasterInputSubsystem::Deinitialize()
{
    ResetInputs();
    Digital.Reset();
    EnhancedAxis.Reset();
    GamepadAxis.Reset();
    OnDispatch.Unbind();
    Super::Deinitialize();
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
    const auto Result=Common.Submit(Event);
    if (Result.status!=RemasterControls::Status::Dispatch) return Result.status;
    if (!OnDispatch.IsBound() || !OnDispatch.Execute(Result.action,Result.target))
    {
        ResetInputs();
        return RemasterControls::Status::Unsupported;
    }
    return Result.status;
}
RemasterControls::Status URemasterInputSubsystem::SubmitPhysical(RemasterControls::Source Source,
    uint16 Control,RemasterControls::Action Action,RemasterControls::Phase Phase)
{
    const auto Packet=Digital.Process(Source,0,Control,Action,Phase,Common.Epoch());
    return Packet.hasEvent ? SubmitEvent(Packet.event) : Packet.status;
}
RemasterControls::Status URemasterInputSubsystem::SubmitAxis(FVector2D Axis,bool bGamepad)
{
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
    return SubmitTouch(Touch.Press(static_cast<unsigned>(Finger),Normalized.X,Normalized.Y,Common.Epoch()));
}
RemasterControls::Status URemasterInputSubsystem::TouchMoved(int32 Finger,FVector2D Normalized)
{
    if (Finger<0) return RemasterControls::Status::Invalid;
    return SubmitTouch(Touch.Move(static_cast<unsigned>(Finger),Normalized.X,Normalized.Y,Common.Epoch()));
}
RemasterControls::Status URemasterInputSubsystem::TouchReleased(int32 Finger)
{
    if (Finger<0) return RemasterControls::Status::Invalid;
    return SubmitTouch(Touch.Release(static_cast<unsigned>(Finger),Common.Epoch()));
}

