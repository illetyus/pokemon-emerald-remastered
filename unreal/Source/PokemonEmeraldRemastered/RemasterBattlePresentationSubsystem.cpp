#include "RemasterBattlePresentationSubsystem.h"

void URemasterBattlePresentationSubsystem::Submit(
    const FRemasterBattlePresentationEvent& Event)
{
    History.Add(Event);
    OnBattlePresentationEvent.Broadcast(Event);
}

void URemasterBattlePresentationSubsystem::Clear()
{
    History.Reset();
}
