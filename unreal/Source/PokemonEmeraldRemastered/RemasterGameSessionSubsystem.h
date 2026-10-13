#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemasterSessionMenu.h"
#include "RemasterTruckSequence.h"
#include "RemasterGameSessionSubsystem.generated.h"

UCLASS()
class POKEMONEMERALDREMASTERED_API URemasterGameSessionSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    bool IsPlaying() const { return Session.Playing(); }
    bool IsBusy() const { return Session.state==RemasterSession::State::Loading || Session.state==RemasterSession::State::Intro; }
    const RemasterSession::MenuFrame& GetMenuFrame() { Menu.Build();return Menu.frame; }
    void Navigate(bool Up) { if(!IsBusy())Menu.Navigate(Up); }
    bool Activate(unsigned Row,uint64 Revision);
    bool ReloadCurrent();
    void BackToTitle();
    bool SetPlayerName(const FString& Value) { return Menu.SetName(TCHAR_TO_UTF8(*Value)); }
    bool SetImportPath(const FString& Value) { return Menu.SetPath(TCHAR_TO_UTF8(*Value)); }
    FVector GetTruckCameraOffset() const;
    FVector GetTruckBoxOffset(int32 LocalId) const;
    uint64 GetGeneration() const { return Session.generation; }
    int32 GetPhase() const { return static_cast<int32>(Session.state); }
private:
    bool BeginLoad();
    bool LoadWorld(bool NewGame);
    bool TickSession(float DeltaSeconds);
    void Fail(const FString& Message);
    bool SetDoor(bool Open);
    void PlayTruckSound(int32 SongId);
    RemasterSession::Lifecycle Session;
    RemasterSession::Menu Menu;
    RemasterSession::TruckSequence Truck;
    FTSTicker::FDelegateHandle TickHandle;
    FDelegateHandle BackgroundHandle,ForegroundHandle;
    double Accumulator=0;
    uint64 IntroGeneration=0;
    bool bBackground=false;
};
