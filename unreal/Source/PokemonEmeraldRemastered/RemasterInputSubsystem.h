#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterTouchInput.h"
#include "RemasterPhysicalInput.h"
#include "RemasterInputSubsystem.generated.h"

DECLARE_DELEGATE_RetVal_TwoParams(bool,FRemasterInputDispatch,
    RemasterControls::Action,RemasterControls::Target);
DECLARE_DELEGATE_RetVal(RemasterControls::Context,FRemasterInputContextReader);
DECLARE_DELEGATE_RetVal_OneParam(bool,FRemasterInputActionOwner,RemasterControls::Action);

// One primary local input owner, matching the existing single core/UI host.
UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterInputSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Deinitialize() override;
    bool AttachRouting(UObject* Owner,const FRemasterInputContextReader& Reader,
        const FRemasterInputDispatch& Dispatch);
    void DetachRouting(const UObject* Owner);
    void SetFieldOwner(const FRemasterInputActionOwner& Owner);
    void SetBattleOwner(const FRemasterInputActionOwner& Owner);
    bool DispatchNativeOwner(RemasterControls::Action Action,RemasterControls::Target Target);
    void RefreshContext();
    void SetContext(RemasterControls::Context Context);
    void ResetInputs();
    bool SetTouchLayout(const RemasterControls::Layout& Layout);
    const RemasterControls::Router& GetRouter() const { return Common; }
    const RemasterControls::Layout& GetTouchLayout() const { return Touch.GetLayout(); }
    RemasterControls::Status SubmitEvent(const RemasterControls::Event& Event);
    RemasterControls::Status SubmitPhysical(RemasterControls::Source Source,
        uint16 Control,RemasterControls::Action Action,RemasterControls::Phase Phase);
    RemasterControls::Status SubmitAxis(FVector2D Axis,bool bGamepad=false);
    RemasterControls::Status TouchPressed(int32 Finger,FVector2D Normalized);
    RemasterControls::Status TouchMoved(int32 Finger,FVector2D Normalized);
    RemasterControls::Status TouchReleased(int32 Finger);
private:
    FRemasterInputDispatch OnDispatch;
    FRemasterInputContextReader OnReadContext;
    FRemasterInputActionOwner FieldOwner,BattleOwner;
    RemasterControls::Status SubmitTouch(const RemasterControls::TouchPacket& Packet);
    RemasterControls::Router Common;
    RemasterControls::Touch Touch;
    RemasterControls::Digital Digital;
    RemasterControls::Analog EnhancedAxis;
    RemasterControls::Analog GamepadAxis{RemasterControls::Source::Gamepad};
};

