#include "R0HUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "RemasterCoreSubsystem.h"

namespace
{
constexpr int32 MapWidth = 8;
constexpr int32 MapHeight = 8;

constexpr uint8 Collision[MapHeight][MapWidth] =
{
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,0,0,1,0,1},
    {1,0,1,0,0,1,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,1,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1}
};
}

void AR0HUD::DrawFilledRect(
    const FVector2D& Position,
    const FVector2D& Size,
    const FLinearColor& Color)
{
    FCanvasTileItem Tile(Position, Size, Color);
    Tile.BlendMode = SE_BLEND_Opaque;
    Canvas->DrawItem(Tile);
}

void AR0HUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas)
    {
        return;
    }

    DrawFilledRect(
        FVector2D::ZeroVector,
        FVector2D(Canvas->SizeX, Canvas->SizeY),
        FLinearColor(0.035f, 0.045f, 0.065f, 1.0f));

    UGameInstance* GameInstance = GetGameInstance();
    URemasterCoreSubsystem* Core = GameInstance
        ? GameInstance->GetSubsystem<URemasterCoreSubsystem>()
        : nullptr;

    if (!Core)
    {
        DrawText(
            TEXT("R0 UNREAL: CORE SUBSYSTEM UNAVAILABLE"),
            FLinearColor::Red,
            40.0f,
            40.0f,
            GEngine ? GEngine->GetMediumFont() : nullptr,
            1.2f,
            false);
        return;
    }

    const FRemasterSnapshot State = Core->Snapshot();

    const float CellSize = FMath::FloorToFloat(
        FMath::Min(
            Canvas->SizeX * 0.65f / static_cast<float>(MapWidth),
            Canvas->SizeY * 0.78f / static_cast<float>(MapHeight)));

    const FVector2D BoardSize(
        CellSize * MapWidth,
        CellSize * MapHeight);

    const FVector2D Origin(
        (Canvas->SizeX - BoardSize.X) * 0.5f,
        (Canvas->SizeY - BoardSize.Y) * 0.5f);

    for (int32 Y = 0; Y < MapHeight; ++Y)
    {
        for (int32 X = 0; X < MapWidth; ++X)
        {
            FLinearColor Color = Collision[Y][X]
                ? FLinearColor(0.22f, 0.25f, 0.29f)
                : FLinearColor(0.22f, 0.58f, 0.27f);

            if (X == 3 && Y == 1)
            {
                Color = (State.EventFlags & 1u)
                    ? FLinearColor(0.95f, 0.65f, 0.12f)
                    : FLinearColor(0.58f, 0.32f, 0.10f);
            }

            if (X == State.TileX && Y == State.TileY)
            {
                Color = State.bEncounterPending
                    ? FLinearColor(0.92f, 0.25f, 0.25f)
                    : FLinearColor(0.16f, 0.48f, 0.95f);
            }

            const FVector2D Position(
                Origin.X + X * CellSize,
                Origin.Y + Y * CellSize);

            DrawFilledRect(
                Position + FVector2D(1.0f, 1.0f),
                FVector2D(CellSize - 2.0f, CellSize - 2.0f),
                Color);
        }
    }

    const FString Status = FString::Printf(
        TEXT("UNREAL + C CORE | tile=(%d,%d) steps=%u flags=0x%X hash=%llu"),
        State.TileX,
        State.TileY,
        State.StepCount,
        State.EventFlags,
        static_cast<unsigned long long>(Core->StateHash()));

    DrawText(
        Status,
        FLinearColor::White,
        24.0f,
        24.0f,
        GEngine ? GEngine->GetSmallFont() : nullptr,
        1.0f,
        false);
}
