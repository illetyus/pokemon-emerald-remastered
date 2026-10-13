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

URemasterUiButton::URemasterUiButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InitIsFocusable(false);
}

void URemasterUiRowWidget::Configure(URemasterUISubsystem* InUI, int32 InIndex,
    int64 InRevision, const FText& Label, bool bEnabled, bool bSelected)
{
    UI = InUI; Index = InIndex; Revision = InRevision;
    SetIsFocusable(false);
    USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
    Size->SetMinDesiredHeight(64.0f);
    WidgetTree->RootWidget = Size;
    UButton* Button = WidgetTree->ConstructWidget<URemasterUiButton>();
    Button->SetIsEnabled(bEnabled);
    Button->SetBackgroundColor(FLinearColor(0.035f,0.09f,0.12f,1.0f));
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
    NameEntry=WidgetTree->ConstructWidget<UEditableTextBox>();
    NameEntry->SetForegroundColor(FLinearColor::Black);
    NameEntry->SetHintText(FText::FromString(TEXT("Adın (1–7 karakter)")));
    NameEntry->SetClearKeyboardFocusOnCommit(true);NameEntry->SetSelectAllTextWhenFocused(true);
    Scrolled->AddChildToVerticalBox(NameEntry);
    NameEntry->OnTextChanged.AddDynamic(this,&URemasterUiWidget::ChangeName);
    NameEntry->OnTextCommitted.AddDynamic(this,&URemasterUiWidget::CommitName);
    PathEntry=WidgetTree->ConstructWidget<UEditableTextBox>();
    PathEntry->SetForegroundColor(FLinearColor::Black);
    PathEntry->SetHintText(FText::FromString(TEXT("Kayıt dosyasının tam yolu")));
    PathEntry->SetClearKeyboardFocusOnCommit(true);PathEntry->SetSelectAllTextWhenFocused(true);
    Scrolled->AddChildToVerticalBox(PathEntry);
    PathEntry->OnTextChanged.AddDynamic(this,&URemasterUiWidget::ChangePath);
    PathEntry->OnTextCommitted.AddDynamic(this,&URemasterUiWidget::CommitPath);
    Rows = WidgetTree->ConstructWidget<UVerticalBox>(); Scrolled->AddChildToVerticalBox(Rows);
    USizeBox* BackSize = WidgetTree->ConstructWidget<USizeBox>(); BackSize->SetMinDesiredHeight(64.0f);
    Content->AddChildToVerticalBox(BackSize);
    BackButton = WidgetTree->ConstructWidget<URemasterUiButton>();
    BackButton->SetBackgroundColor(FLinearColor(0.035f,0.09f,0.12f,1.0f));
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
    BackButton->SetVisibility(Frame.bModal && Frame.ScreenId!=1000 && Frame.ScreenId!=1003 && Frame.ScreenId!=1004 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    NameEntry->SetVisibility(Frame.ScreenId==1001?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    PathEntry->SetVisibility(Frame.ScreenId==1002?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    EditorRevision=Frame.Revision;
    if(EditorScreen!=Frame.ScreenId) {
        EditorScreen=Frame.ScreenId;
        NameEntry->SetText(FText::FromString(Frame.SessionName));PathEntry->SetText(FText::FromString(Frame.ImportPath));
    }
    if(!HasSessionTextFocus() && Frame.ScreenId==1001 && NameEntry->GetText().ToString()!=Frame.SessionName)
        NameEntry->SetText(FText::FromString(Frame.SessionName));
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

bool URemasterUiWidget::HasSessionTextFocus() const {
    const auto Focused=[](const UEditableTextBox* Entry){return Entry && Entry->GetVisibility()==ESlateVisibility::Visible && (Entry->HasAnyUserFocus() || Entry->HasFocusedDescendants());};
    return Focused(NameEntry)||Focused(PathEntry);
}
void URemasterUiWidget::CommitName(const FText& Text,ETextCommit::Type) {
    if(UI && EditorScreen==1001){UI->NoteSessionTextCommit();UI->SetSessionText(Text.ToString(),true,EditorRevision);}
}
void URemasterUiWidget::CommitPath(const FText& Text,ETextCommit::Type) {
    if(UI && EditorScreen==1002){UI->NoteSessionTextCommit();UI->SetSessionText(Text.ToString(),false,EditorRevision);}
}

void URemasterUiWidget::ChangeName(const FText& Text) { if(UI && EditorScreen==1001)UI->SetSessionText(Text.ToString(),true,EditorRevision); }
void URemasterUiWidget::ChangePath(const FText& Text) { if(UI && EditorScreen==1002)UI->SetSessionText(Text.ToString(),false,EditorRevision); }
