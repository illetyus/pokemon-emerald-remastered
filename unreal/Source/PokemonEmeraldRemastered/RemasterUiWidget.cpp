#include "RemasterUiWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/SafeZone.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameInstance.h"

void URemasterUiRowWidget::Configure(URemasterUISubsystem* InUI, int32 InIndex,
    int64 InRevision, const FText& Label, bool bEnabled, bool bSelected)
{
    UI = InUI; Index = InIndex; Revision = InRevision;
    SetIsFocusable(false);
    USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
    Size->SetMinDesiredHeight(64.0f);
    WidgetTree->RootWidget = Size;
    UButton* Button = WidgetTree->ConstructWidget<UButton>();
    Button->SetIsFocusable(false);
    Button->SetIsEnabled(bEnabled);
    Size->AddChild(Button);
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString((bSelected ? TEXT("› ") : TEXT("")) + Label.ToString()));
    Text->SetAutoWrapText(true);
    Button->AddChild(Text);
    Button->OnClicked.AddDynamic(this, &URemasterUiRowWidget::Click);
}

void URemasterUiRowWidget::Click()
{
    if (UI) UI->ActivateRow(Index, Revision);
}

void URemasterUiWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);
    // Safe area and scrollable content work in portrait/landscape without assets.
    USafeZone* Safe = WidgetTree->ConstructWidget<USafeZone>();
    WidgetTree->RootWidget = Safe;
    USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
    Width->SetMaxDesiredWidth(600.0f);
    Safe->AddChild(Width);
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetPadding(FMargin(16.0f));
    Panel->SetBrushColor(FLinearColor(0.025f, 0.055f, 0.07f, 0.9f));
    Width->AddChild(Panel);
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Panel->AddChild(Content);
    Title = WidgetTree->ConstructWidget<UTextBlock>();
    Title->SetAutoWrapText(true); Content->AddChildToVerticalBox(Title);
    UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>();
    Hint->SetText(FText::FromString(TEXT("↑ ↓ Seç · A / Enter Onay · B / Esc Geri")));
    Content->AddChildToVerticalBox(Hint);
    Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
    Content->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UVerticalBox* Scrolled = WidgetTree->ConstructWidget<UVerticalBox>();
    Scroll->AddChild(Scrolled);
    Body = WidgetTree->ConstructWidget<UTextBlock>();
    Body->SetAutoWrapText(true); Scrolled->AddChildToVerticalBox(Body);
    // Source Hoenn region grid is 28 × 15 (region_map.c). No synthetic world map.
    MapPanel = WidgetTree->ConstructWidget<USizeBox>();
    MapPanel->SetWidthOverride(336.0f); MapPanel->SetHeightOverride(180.0f);
    Scrolled->AddChildToVerticalBox(MapPanel);
    UBorder* MapBackground = WidgetTree->ConstructWidget<UBorder>();
    MapBackground->SetBrushColor(FLinearColor(0.1f, 0.22f, 0.25f, 1.0f));
    MapPanel->AddChild(MapBackground);
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    MapBackground->AddChild(Canvas);
    Marker = WidgetTree->ConstructWidget<UBorder>();
    Marker->SetBrushColor(FLinearColor(1.0f, 0.8f, 0.1f, 1.0f));
    MarkerSlot = Canvas->AddChildToCanvas(Marker);
    Rows = WidgetTree->ConstructWidget<UVerticalBox>(); Scrolled->AddChildToVerticalBox(Rows);
    USizeBox* BackSize = WidgetTree->ConstructWidget<USizeBox>(); BackSize->SetMinDesiredHeight(64.0f);
    Content->AddChildToVerticalBox(BackSize);
    BackButton = WidgetTree->ConstructWidget<UButton>(); BackButton->SetIsFocusable(false);
    BackSize->AddChild(BackButton);
    UTextBlock* BackText = WidgetTree->ConstructWidget<UTextBlock>();
    BackText->SetText(FText::FromString(TEXT("Geri"))); BackButton->AddChild(BackText);
    BackButton->OnClicked.AddDynamic(this, &URemasterUiWidget::Back);
}

void URemasterUiWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RowsRevision = -1;
    UI = GetGameInstance()->GetSubsystem<URemasterUISubsystem>();
    if (UI)
    {
        UI->OnFrameChanged.AddDynamic(this, &URemasterUiWidget::Present);
        Present(UI->GetFrame());
    }
}

void URemasterUiWidget::NativeDestruct()
{
    if (UI) UI->OnFrameChanged.RemoveDynamic(this, &URemasterUiWidget::Present);
    Super::NativeDestruct();
}

void URemasterUiWidget::Present(const FRemasterUiFrame& Frame)
{
    Title->SetText(Frame.Title); Body->SetText(Frame.Body);
    BackButton->SetVisibility(Frame.bModal ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    const bool bMap = Frame.ScreenId == static_cast<int32>(RemasterUi::Screen::Map);
    MapPanel->SetVisibility(bMap ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    const bool bMarker = Frame.bHasMarker && Frame.MarkerPosition.X >= 0 && Frame.MarkerPosition.Y >= 0
        && Frame.MarkerSize.X > 0 && Frame.MarkerSize.Y > 0
        && Frame.MarkerPosition.X + Frame.MarkerSize.X <= 28
        && Frame.MarkerPosition.Y + Frame.MarkerSize.Y <= 15;
    Marker->SetVisibility(bMarker ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (bMarker)
    {
        MarkerSlot->SetPosition(Frame.MarkerPosition * 12.0f);
        MarkerSlot->SetSize(Frame.MarkerSize * 12.0f);
    }
    if (RowsRevision == Frame.Revision) return; // Text reveal does not rebuild pointer targets.
    RowsRevision = Frame.Revision;
    Rows->ClearChildren();
    for (int32 i = 0; i < Frame.Rows.Num(); ++i)
    {
        auto* Row = CreateWidget<URemasterUiRowWidget>(GetGameInstance(), URemasterUiRowWidget::StaticClass());
        if (!Row) continue;
        Row->Configure(UI, i, Frame.Revision, Frame.Rows[i].Label, Frame.Rows[i].bEnabled, i == Frame.Selected);
        Rows->AddChildToVerticalBox(Row);
        if (Frame.bModal && i == Frame.Selected)
            Scroll->ScrollWidgetIntoView(Row, false, EDescendantScrollDestination::IntoView);
    }
}

void URemasterUiWidget::Back()
{
    if (UI) UI->SubmitAction(ERemasterUiAction::Cancel);
}
