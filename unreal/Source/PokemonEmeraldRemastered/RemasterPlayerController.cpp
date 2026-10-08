#include "RemasterPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "RemasterInputConfig.h"
#include "RemasterInputSubsystem.h"
#include "RemasterInputDispatch.h"
#include "RemasterUISubsystem.h"
#include "RemasterOverworldPawn.h"

extern "C"
{
#include "remaster/emerald_map.h"
}

void ARemasterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (InputConfig && InputConfig->TouchRectangles.Num() > 0)
    {
        RemasterControls::Layout Layout;
        bool bValid = InputConfig->TouchRectangles.Num() == 10;
        if (bValid) for (int32 i = 0; i < 10; ++i)
        {
            const auto& Rect = InputConfig->TouchRectangles[i];
            Layout.rects[i] = {Rect.X, Rect.Y, Rect.Z, Rect.W};
        }
        auto* Input = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr;
        if (!bValid || !Input || !Input->SetTouchLayout(Layout))
            UE_LOG(LogTemp, Warning, TEXT("Invalid touch layout: retaining previous validated layout"));
    }

    if (InputConfig)
    {
        if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                    LocalPlayer->GetSubsystem<
                        UEnhancedInputLocalPlayerSubsystem>())
            {
                if (UInputMappingContext* Context =
                        InputConfig->MappingContext.LoadSynchronous())
                {
                    if (!Subsystem->HasMappingContext(Context))
                    {
                        FModifyContextOptions Options;Options.bIgnoreAllPressedKeysUntilRelease=true;
                        Subsystem->AddMappingContext(Context,0,Options);
                        OwnedMappingContext=Context;
                    }
                }
            }
        }
    }

    if (ARemasterOverworldPawn* OverworldPawn =
            Cast<ARemasterOverworldPawn>(GetPawn()))
    {
        OverworldPawn->SyncFromAuthoritativeState();
    }
    if (auto* UI = GetGameInstance()->GetSubsystem<URemasterUISubsystem>())
        UI->StartNativePresentation();
    if (auto* Input=GetGameInstance()->GetSubsystem<URemasterInputSubsystem>())
    {
        bInputRoutingAttached=Input->AttachRouting(this,
            FRemasterInputContextReader::CreateUObject(this,&ARemasterPlayerController::ReadInputContext),
            FRemasterInputDispatch::CreateUObject(this,&ARemasterPlayerController::DispatchInput));
        if (!bInputRoutingAttached)
            UE_LOG(LogTemp,Warning,TEXT("Input routing already has a different native owner"));
    }
}
void ARemasterPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bInputRoutingAttached=false;
    if (auto* Input=GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr)
        Input->DetachRouting(this);
    if (OwnedMappingContext.IsValid())
        if (auto* Local=GetLocalPlayer())
            if (auto* Enhanced=Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                Enhanced->RemoveMappingContext(OwnedMappingContext.Get());
    OwnedMappingContext.Reset();
    Super::EndPlay(EndPlayReason);
}
RemasterControls::Context ARemasterPlayerController::ReadInputContext()
{
    if (auto* UI=GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterUISubsystem>() : nullptr)
        return UI->ReadInputContext();
    return {};
}
bool ARemasterPlayerController::DispatchInput(RemasterControls::Action Action,RemasterControls::Target Target)
{
    using namespace RemasterControls;
    auto UI=[this](RemasterControls::Action A){return RouteUI(static_cast<ERemasterUiAction>(A));};
    auto World=[this](RemasterControls::Action A)
    {
        const int32 Directions[]={REMASTER_EMERALD_DIR_NORTH,REMASTER_EMERALD_DIR_SOUTH,
            REMASTER_EMERALD_DIR_WEST,REMASTER_EMERALD_DIR_EAST};
        return StepDirection(Directions[static_cast<unsigned>(A)]);
    };
    auto Field=[this](RemasterControls::Action A)
    {
        auto* Input=GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr;
        return Input && Input->DispatchNativeOwner(A,RemasterControls::Target::FieldInteract);
    };
    auto Battle=[this](RemasterControls::Action A)
    {
        auto* Input=GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr;
        return Input && Input->DispatchNativeOwner(A,RemasterControls::Target::BattleInput);
    };
    return Deliver(Action,Target,UI,World,Field,Battle);
}

void ARemasterPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!InputComponent)
        return;

    if (!InputConfig || InputConfig->bTouchEnabled)
    {
        InputComponent->BindTouch(IE_Pressed, this, &ARemasterPlayerController::TouchPressed);
        InputComponent->BindTouch(IE_Repeat, this, &ARemasterPlayerController::TouchMoved);
        InputComponent->BindTouch(IE_Released, this, &ARemasterPlayerController::TouchReleased);
        bEnableTouchEvents = true;
    }

    bool MoveBound = false, InteractBound = false, CancelBound = false;
    bool MenuBound = false, MapBound = false, QuestBound = false, QuickBound = false;
    auto* Enhanced = Cast<UEnhancedInputComponent>(InputComponent);
    TSet<UInputAction*> BoundActions;
    UInputMappingContext* Mapping = InputConfig ? InputConfig->MappingContext.LoadSynchronous() : nullptr;
    auto IsMapped = [Mapping](UInputAction* Action)
    {
        if (!Mapping || !Action) return false;
        for (const auto& Entry : Mapping->GetMappings())
            if (Entry.Action == Action) return true;
        return false;
    };
    if (Enhanced && InputConfig && Mapping)
    {
        if (UInputAction* Action = InputConfig->Move.LoadSynchronous())
        {
            if (Action->ValueType == EInputActionValueType::Axis2D && IsMapped(Action))
            {
                Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleMove);
                Enhanced->BindAction(Action, ETriggerEvent::Triggered, this, &ARemasterPlayerController::HandleMove);
                Enhanced->BindAction(Action, ETriggerEvent::Completed, this, &ARemasterPlayerController::HandleMoveReleased);
                Enhanced->BindAction(Action, ETriggerEvent::Canceled, this, &ARemasterPlayerController::HandleMoveReleased);
                BoundActions.Add(Action);
                MoveBound = true;
            }
        }
        auto BindButton = [this, Enhanced, &BoundActions, &IsMapped](UInputAction* Action, RemasterControls::Action Semantic)
        {
            if (!Action || Action->ValueType != EInputActionValueType::Boolean
                || !IsMapped(Action) || BoundActions.Contains(Action))
                return false;
            Enhanced->BindAction(Action, ETriggerEvent::Started, this, &ARemasterPlayerController::HandleButton,
                Semantic, RemasterControls::Phase::Pressed);
            Enhanced->BindAction(Action, ETriggerEvent::Completed, this, &ARemasterPlayerController::HandleButton,
                Semantic, RemasterControls::Phase::Released);
            Enhanced->BindAction(Action, ETriggerEvent::Canceled, this, &ARemasterPlayerController::HandleButton,
                Semantic, RemasterControls::Phase::Canceled);
            BoundActions.Add(Action);
            return true;
        };
        InteractBound = BindButton(InputConfig->Interact.LoadSynchronous(), RemasterControls::Action::Confirm);
        CancelBound = BindButton(InputConfig->Cancel.LoadSynchronous(), RemasterControls::Action::Cancel);
        MenuBound = BindButton(InputConfig->Menu.LoadSynchronous(), RemasterControls::Action::Menu);
        MapBound = BindButton(InputConfig->Map.LoadSynchronous(), RemasterControls::Action::Map);
        QuestBound = BindButton(InputConfig->Quest.LoadSynchronous(), RemasterControls::Action::Quest);
        QuickBound = BindButton(InputConfig->QuickItem.LoadSynchronous(), RemasterControls::Action::QuickItem);
    }
    if (!MoveBound)
    {
        BindNativeAction(RemasterControls::Action::Up);
        BindNativeAction(RemasterControls::Action::Down);
        BindNativeAction(RemasterControls::Action::Left);
        BindNativeAction(RemasterControls::Action::Right);
    }
    if (!InteractBound) BindNativeAction(RemasterControls::Action::Confirm);
    if (!CancelBound) BindNativeAction(RemasterControls::Action::Cancel);
    if (!MenuBound) BindNativeAction(RemasterControls::Action::Menu);
    if (!MapBound) BindNativeAction(RemasterControls::Action::Map);
    if (!QuestBound) BindNativeAction(RemasterControls::Action::Quest);
    if (!QuickBound) BindNativeAction(RemasterControls::Action::QuickItem);
    bNativeGamepadAxis = !MoveBound;
}

void ARemasterPlayerController::BindNativeAction(RemasterControls::Action Action)
{
    for (const auto& Binding : RemasterControls::Bindings)
    {
        if (Binding.action != Action) continue;
        const FKey Key(FName(UTF8_TO_TCHAR(Binding.engineKey)));
        for (EInputEvent Event : {IE_Pressed, IE_Released})
        {
            FInputKeyBinding Native(FInputChord(Key), Event);
            Native.KeyDelegate.GetDelegateForManualSet().BindUObject(this,
                &ARemasterPlayerController::PhysicalInput, Binding.source, Binding.control, Action,
                Event == IE_Pressed ? RemasterControls::Phase::Pressed : RemasterControls::Phase::Released);
            InputComponent->KeyBindings.Add(MoveTemp(Native));
        }
    }
}
void ARemasterPlayerController::PhysicalInput(RemasterControls::Source Source,uint16 Control,
    RemasterControls::Action Action,RemasterControls::Phase Phase)
{
    if (!bInputRoutingAttached) return;
    if (auto* Input = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr)
        Input->SubmitPhysical(Source, Control, Action, Phase);
}
void ARemasterPlayerController::HandleButton(const FInputActionValue&,
    RemasterControls::Action Action,RemasterControls::Phase Phase)
{
    PhysicalInput(RemasterControls::Source::Enhanced,32+static_cast<uint16>(Action),Action,Phase);
}
void ARemasterPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (!bInputRoutingAttached) return;
    if (auto* Input=GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr)
        Input->RefreshContext();
    if (bNativeGamepadAxis)
        if (auto* Input = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr)
            Input->SubmitAxis(FVector2D(GetInputAnalogKeyState(EKeys::Gamepad_LeftX),
                GetInputAnalogKeyState(EKeys::Gamepad_LeftY)), true);
}

bool ARemasterPlayerController::StepDirection(int32 Direction)
{
    const ERemasterUiAction UiDirection = Direction == REMASTER_EMERALD_DIR_NORTH ? ERemasterUiAction::Up
        : Direction == REMASTER_EMERALD_DIR_SOUTH ? ERemasterUiAction::Down
        : Direction == REMASTER_EMERALD_DIR_WEST ? ERemasterUiAction::Left : ERemasterUiAction::Right;
    if (RouteUI(UiDirection)) return true;
    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return false;

    URemasterWorldGameplaySubsystem* Gameplay =
        GI->GetSubsystem<URemasterWorldGameplaySubsystem>();

    if (!Gameplay || !Gameplay->IsMapReady())
        return false;

    FRemasterPlayerStepResult Result;
    if (!Gameplay->StepPlayer(Direction, Result))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Authoritative overworld step failed: direction=%d"),
            Direction);
        return false;
    }

    if (ARemasterOverworldPawn* OverworldPawn =
            Cast<ARemasterOverworldPawn>(GetPawn()))
    {
        OverworldPawn->ApplyAuthoritativeStep(Result);
    }

    if (auto* UI = GI->GetSubsystem<URemasterUISubsystem>())
        UI->NotifyCoreStep(!Result.ScriptId.IsEmpty(), Result.bEncounterPending, Result.bRepelWoreOff);
    BP_OnWorldStep(Result);
    return true;
}

void ARemasterPlayerController::HandleMove(const FInputActionValue& Value)
{
    if (!bInputRoutingAttached) return;
    if (auto* Input = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr)
        Input->SubmitAxis(Value.Get<FVector2D>());
}
void ARemasterPlayerController::HandleMoveReleased(const FInputActionValue&)
{
    if (!bInputRoutingAttached) return;
    if (auto* Input = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr)
        Input->SubmitAxis(FVector2D::ZeroVector);
}

bool ARemasterPlayerController::RouteUI(ERemasterUiAction Action)
{
    if (auto* UI = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterUISubsystem>() : nullptr)
        return UI->SubmitAction(Action);
    return false;
}
FVector2D ARemasterPlayerController::NormalizeTouch(FVector Location) const
{
    int32 Width = 0, Height = 0;
    GetViewportSize(Width, Height);
    const FVector4 Inset = InputConfig ? InputConfig->TouchSafeInsets : FVector4(0,0,0,0);
    const auto Point = RemasterControls::NormalizePixel(Location.X,Location.Y,Width,Height,
        {Inset.X,Inset.Y,Inset.Z,Inset.W});
    return Point.valid ? FVector2D(Point.x,Point.y) : FVector2D(-1,-1);
}
void ARemasterPlayerController::TouchPressed(ETouchIndex::Type Finger,FVector Location)
{
    if (!bInputRoutingAttached) return;
    if (auto* Input = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr)
        Input->TouchPressed(static_cast<int32>(Finger),NormalizeTouch(Location));
}
void ARemasterPlayerController::TouchMoved(ETouchIndex::Type Finger,FVector Location)
{
    if (!bInputRoutingAttached) return;
    if (auto* Input = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr)
        Input->TouchMoved(static_cast<int32>(Finger),NormalizeTouch(Location));
}
void ARemasterPlayerController::TouchReleased(ETouchIndex::Type Finger,FVector)
{
    if (!bInputRoutingAttached) return;
    if (auto* Input = GetGameInstance() ? GetGameInstance()->GetSubsystem<URemasterInputSubsystem>() : nullptr)
        Input->TouchReleased(static_cast<int32>(Finger));
}


