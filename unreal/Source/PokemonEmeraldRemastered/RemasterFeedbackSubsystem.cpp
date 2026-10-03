#include "RemasterFeedbackSubsystem.h"

void URemasterFeedbackSubsystem::Emit(
    FName EventName,
    int64 Arg0,
    int64 Arg1)
{
    OnFeedback.Broadcast(EventName, Arg0, Arg1);
}
