#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemasterEnvironmentController.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class USceneComponent;
class USkyLightComponent;

UCLASS()
class POKEMONEMERALDREMASTERED_API ARemasterEnvironmentController
    : public AActor
{
    GENERATED_BODY()

public:
    ARemasterEnvironmentController();

    UFUNCTION(BlueprintCallable, Category="Remaster|Environment")
    void SetTimeOfDay(float Hour);

    UFUNCTION(BlueprintCallable, Category="Remaster|Environment")
    void SetWeatherTag(FName InWeather);

    UFUNCTION(BlueprintPure, Category="Remaster|Environment")
    float GetTimeOfDay() const
    {
        return TimeOfDay;
    }

    UFUNCTION(BlueprintPure, Category="Remaster|Environment")
    FName GetWeatherTag() const
    {
        return WeatherTag;
    }

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UDirectionalLightComponent> Sun;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USkyLightComponent> Sky;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UExponentialHeightFogComponent> Fog;

    UPROPERTY(EditAnywhere, Category="Remaster|Environment")
    float DaySunIntensity = 4.0f;

    UPROPERTY(EditAnywhere, Category="Remaster|Environment")
    float NightSunIntensity = 0.15f;

private:
    float TimeOfDay = 12.0f;
    FName WeatherTag = TEXT("Clear");
};
