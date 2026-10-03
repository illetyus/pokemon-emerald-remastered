#include "RemasterObjectiveMarker.h"

#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"

ARemasterObjectiveMarker::ARemasterObjectiveMarker()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
    Text->SetupAttachment(SceneRoot);
    Text->SetText(FText::FromString(TEXT("!")));
    Text->SetHorizontalAlignment(EHTA_Center);
    Text->SetWorldSize(48.0f);
    Text->SetTextRenderColor(FColor(255, 210, 50));
}

void ARemasterObjectiveMarker::SetMarkerVisible(bool bVisible)
{
    SetActorHiddenInGame(!bVisible);
}

void ARemasterObjectiveMarker::SetMarkerText(const FText& InText)
{
    Text->SetText(InText);
}
