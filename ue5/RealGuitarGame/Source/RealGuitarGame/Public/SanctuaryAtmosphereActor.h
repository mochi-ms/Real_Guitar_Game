#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GuitarPerformanceTypes.h"
#include "SanctuaryAtmosphereActor.generated.h"

struct FGuitarSettingsData;
class UAudioComponent;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UPointLightComponent;
class USkyLightComponent;
class USoundBase;

/**
 * Controls time-of-day, lighting warmth, fog, bench lamp glow, and ambient audio
 * in response to ESanctuaryMood presets (GoldenSunset, CozyNightLamp, MorningMist, RainyLoFi).
 */
UCLASS()
class REALGUITARGAME_API ASanctuaryAtmosphereActor : public AActor
{
    GENERATED_BODY()

public:
    ASanctuaryAtmosphereActor();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere|Lighting")
    TObjectPtr<UDirectionalLightComponent> SunLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere|Lighting")
    TObjectPtr<USkyLightComponent> SkyLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere|Lighting")
    TObjectPtr<UExponentialHeightFogComponent> HeightFog;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere|Lighting")
    TObjectPtr<UPointLightComponent> WarmBenchLamp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere|Audio")
    TObjectPtr<UAudioComponent> AmbientAudioComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Audio")
    TObjectPtr<USoundBase> AmbientLoopSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
    float TransitionSpeed = 2.4f;

    UFUNCTION(BlueprintCallable, Category = "Atmosphere")
    void ApplyMoodPreset(ESanctuaryMood Mood, bool bImmediate = false);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    struct FMoodLightingTarget
    {
        FRotator SunRotation = FRotator(-16.0f, -38.0f, 0.0f);
        float SunIntensity = 5.5f;
        FLinearColor SunColor = FLinearColor(1.0f, 0.72f, 0.45f);
        float SkyIntensity = 1.0f;
        FLinearColor SkyColor = FLinearColor(0.85f, 0.78f, 0.72f);
        float FogDensity = 0.015f;
        FLinearColor FogColor = FLinearColor(0.75f, 0.58f, 0.45f);
        float LampIntensity = 1800.0f;
        FLinearColor LampColor = FLinearColor(1.0f, 0.68f, 0.35f);
        float AmbientMoodGain = 0.7f;
    };

    static FMoodLightingTarget GetPresetTarget(ESanctuaryMood Mood);

    UFUNCTION()
    void HandleSettingsChanged(const FGuitarSettingsData& NewSettings);

    void ApplyTargetsImmediate();
    void RefreshAmbientVolume(const FGuitarSettingsData& Settings);

    FMoodLightingTarget TargetState;
    FMoodLightingTarget CurrentState;
    bool bInterpolating = false;
};
