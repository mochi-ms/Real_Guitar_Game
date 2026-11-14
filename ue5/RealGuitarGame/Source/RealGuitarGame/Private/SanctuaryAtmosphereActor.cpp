#include "SanctuaryAtmosphereActor.h"

#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GuitarGameSettings.h"

ASanctuaryAtmosphereActor::ASanctuaryAtmosphereActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
    SunLight->SetupAttachment(Root);
    SunLight->SetMobility(EComponentMobility::Movable);
    SunLight->SetRelativeRotation(FRotator(-15.0f, -38.0f, 0.0f));
    SunLight->SetIntensity(5.5f);
    SunLight->SetAtmosphereSunLight(true);

    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
    SkyLight->SetupAttachment(Root);
    SkyLight->SetMobility(EComponentMobility::Movable);
    SkyLight->bRealTimeCapture = true;

    HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
    HeightFog->SetupAttachment(Root);
    HeightFog->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
    HeightFog->SetFogDensity(0.014f);

    WarmBenchLamp = CreateDefaultSubobject<UPointLightComponent>(TEXT("WarmBenchLamp"));
    WarmBenchLamp->SetupAttachment(Root);
    WarmBenchLamp->SetRelativeLocation(FVector(0.0f, 0.0f, 250.0f));
    WarmBenchLamp->SetAttenuationRadius(780.0f);
    WarmBenchLamp->SetSourceRadius(16.0f);
    WarmBenchLamp->SetIntensity(2200.0f);
    WarmBenchLamp->SetLightColor(FLinearColor(1.0f, 0.72f, 0.40f));

    CampfireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CampfireLight"));
    CampfireLight->SetupAttachment(Root);
    CampfireLight->SetRelativeLocation(FVector(-220.0f, -290.0f, 68.0f));
    CampfireLight->SetAttenuationRadius(920.0f);
    CampfireLight->SetSourceRadius(18.0f);
    CampfireLight->SetIntensity(2200.0f);
    CampfireLight->SetLightColor(FLinearColor(1.0f, 0.46f, 0.14f));

    AmbientAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientAudio"));
    AmbientAudioComponent->SetupAttachment(Root);
    AmbientAudioComponent->bAutoActivate = false;
    AmbientAudioComponent->bIsUISound = false;
}

ASanctuaryAtmosphereActor::FMoodLightingTarget ASanctuaryAtmosphereActor::GetPresetTarget(ESanctuaryMood Mood)
{
    FMoodLightingTarget T;
    switch (Mood)
    {
    case ESanctuaryMood::CozyNightLamp:
        T.SunRotation = FRotator(-42.0f, 115.0f, 0.0f);
        T.SunIntensity = 0.45f;
        T.SunColor = FLinearColor(0.42f, 0.56f, 0.92f);
        T.SkyIntensity = 0.35f;
        T.SkyColor = FLinearColor(0.28f, 0.36f, 0.62f);
        T.FogDensity = 0.018f;
        T.FogColor = FLinearColor(0.14f, 0.18f, 0.32f);
        T.LampIntensity = 5200.0f;
        T.LampColor = FLinearColor(1.0f, 0.62f, 0.28f);
        T.CampfireIntensity = 4200.0f;
        T.AmbientMoodGain = 0.85f;
        break;

    case ESanctuaryMood::MorningMist:
        T.SunRotation = FRotator(-24.0f, 45.0f, 0.0f);
        T.SunIntensity = 6.2f;
        T.SunColor = FLinearColor(0.96f, 0.94f, 0.88f);
        T.SkyIntensity = 1.25f;
        T.SkyColor = FLinearColor(0.76f, 0.88f, 1.0f);
        T.FogDensity = 0.026f;
        T.FogColor = FLinearColor(0.72f, 0.82f, 0.90f);
        T.LampIntensity = 300.0f;
        T.LampColor = FLinearColor(1.0f, 0.78f, 0.52f);
        T.CampfireIntensity = 900.0f;
        T.AmbientMoodGain = 0.65f;
        break;

    case ESanctuaryMood::RainyLoFi:
        T.SunRotation = FRotator(-50.0f, -20.0f, 0.0f);
        T.SunIntensity = 1.6f;
        T.SunColor = FLinearColor(0.55f, 0.66f, 0.78f);
        T.SkyIntensity = 0.72f;
        T.SkyColor = FLinearColor(0.45f, 0.55f, 0.68f);
        T.FogDensity = 0.035f;
        T.FogColor = FLinearColor(0.32f, 0.40f, 0.50f);
        T.LampIntensity = 3600.0f;
        T.LampColor = FLinearColor(1.0f, 0.70f, 0.38f);
        T.CampfireIntensity = 2600.0f;
        T.AmbientMoodGain = 1.0f;
        break;

    case ESanctuaryMood::GoldenSunset:
    default:
        T.SunRotation = FRotator(-13.0f, -42.0f, 0.0f);
        T.SunIntensity = 5.8f;
        T.SunColor = FLinearColor(1.0f, 0.66f, 0.36f);
        T.SkyIntensity = 0.95f;
        T.SkyColor = FLinearColor(0.95f, 0.76f, 0.64f);
        T.FogDensity = 0.014f;
        T.FogColor = FLinearColor(0.84f, 0.58f, 0.42f);
        T.LampIntensity = 1900.0f;
        T.LampColor = FLinearColor(1.0f, 0.68f, 0.35f);
        T.CampfireIntensity = 2100.0f;
        T.AmbientMoodGain = 0.70f;
        break;
    }
    return T;
}

void ASanctuaryAtmosphereActor::BeginPlay()
{
    Super::BeginPlay();

    ESanctuaryMood InitialMood = ESanctuaryMood::GoldenSunset;
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            SettingsSub->OnSettingsChanged.AddDynamic(this, &ASanctuaryAtmosphereActor::HandleSettingsChanged);
            InitialMood = SettingsSub->GetSettings().AtmosphereMood;
            RefreshAmbientVolume(SettingsSub->GetSettings());
        }
    }

    if (AmbientAudioComponent && AmbientLoopSound)
    {
        AmbientAudioComponent->SetSound(AmbientLoopSound);
        AmbientAudioComponent->Play();
    }

    ApplyMoodPreset(InitialMood, true);
}

void ASanctuaryAtmosphereActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            SettingsSub->OnSettingsChanged.RemoveDynamic(this, &ASanctuaryAtmosphereActor::HandleSettingsChanged);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void ASanctuaryAtmosphereActor::ApplyMoodPreset(ESanctuaryMood Mood, bool bImmediate)
{
    TargetState = GetPresetTarget(Mood);
    if (bImmediate)
    {
        CurrentState = TargetState;
        ApplyTargetsImmediate();
        bInterpolating = false;
    }
    else
    {
        bInterpolating = true;
    }
}

void ASanctuaryAtmosphereActor::HandleSettingsChanged(const FGuitarSettingsData& NewSettings)
{
    ApplyMoodPreset(NewSettings.AtmosphereMood, false);
    RefreshAmbientVolume(NewSettings);
}

void ASanctuaryAtmosphereActor::RefreshAmbientVolume(const FGuitarSettingsData& Settings)
{
    if (!AmbientAudioComponent)
    {
        return;
    }
    const float Vol = FMath::Clamp(Settings.MasterVolume * Settings.AmbientVolume * TargetState.AmbientMoodGain, 0.0f, 1.0f);
    AmbientAudioComponent->SetVolumeMultiplier(Vol);
}

void ASanctuaryAtmosphereActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FlickerTime += DeltaTime;
    const float EmberMod = 1.0f + 0.14f * FMath::Sin(FlickerTime * 8.4f) + 0.08f * FMath::Sin(FlickerTime * 16.9f + 1.1f);
    if (CampfireLight)
    {
        CampfireLight->SetIntensity(CurrentState.CampfireIntensity * EmberMod);
    }

    if (!bInterpolating)
    {
        return;
    }

    const float Speed = FMath::Max(0.5f, TransitionSpeed);
    CurrentState.SunRotation = FMath::RInterpTo(CurrentState.SunRotation, TargetState.SunRotation, DeltaTime, Speed);
    CurrentState.SunIntensity = FMath::FInterpTo(CurrentState.SunIntensity, TargetState.SunIntensity, DeltaTime, Speed);
    CurrentState.SunColor = FMath::CInterpTo(CurrentState.SunColor, TargetState.SunColor, DeltaTime, Speed);
    CurrentState.SkyIntensity = FMath::FInterpTo(CurrentState.SkyIntensity, TargetState.SkyIntensity, DeltaTime, Speed);
    CurrentState.SkyColor = FMath::CInterpTo(CurrentState.SkyColor, TargetState.SkyColor, DeltaTime, Speed);
    CurrentState.FogDensity = FMath::FInterpTo(CurrentState.FogDensity, TargetState.FogDensity, DeltaTime, Speed);
    CurrentState.FogColor = FMath::CInterpTo(CurrentState.FogColor, TargetState.FogColor, DeltaTime, Speed);
    CurrentState.LampIntensity = FMath::FInterpTo(CurrentState.LampIntensity, TargetState.LampIntensity, DeltaTime, Speed);
    CurrentState.LampColor = FMath::CInterpTo(CurrentState.LampColor, TargetState.LampColor, DeltaTime, Speed);
    CurrentState.CampfireIntensity = FMath::FInterpTo(CurrentState.CampfireIntensity, TargetState.CampfireIntensity, DeltaTime, Speed);
    CurrentState.AmbientMoodGain = FMath::FInterpTo(CurrentState.AmbientMoodGain, TargetState.AmbientMoodGain, DeltaTime, Speed);

    ApplyTargetsImmediate();

    if (UGameInstance* GI = GetGameInstance())
    {
        if (const UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            RefreshAmbientVolume(SettingsSub->GetSettings());
        }
    }

    const bool bDone =
        FMath::IsNearlyEqual(CurrentState.SunIntensity, TargetState.SunIntensity, 0.02f) &&
        FMath::IsNearlyEqual(CurrentState.LampIntensity, TargetState.LampIntensity, 2.0f) &&
        FMath::IsNearlyEqual(CurrentState.CampfireIntensity, TargetState.CampfireIntensity, 2.0f) &&
        FMath::IsNearlyEqual(CurrentState.FogDensity, TargetState.FogDensity, 0.0005f);
    if (bDone)
    {
        CurrentState = TargetState;
        ApplyTargetsImmediate();
        bInterpolating = false;
    }
}

void ASanctuaryAtmosphereActor::ApplyTargetsImmediate()
{
    if (SunLight)
    {
        SunLight->SetRelativeRotation(CurrentState.SunRotation);
        SunLight->SetIntensity(CurrentState.SunIntensity);
        SunLight->SetLightColor(CurrentState.SunColor);
    }
    if (SkyLight)
    {
        SkyLight->SetIntensity(CurrentState.SkyIntensity);
        SkyLight->SetLightColor(CurrentState.SkyColor);
    }
    if (HeightFog)
    {
        HeightFog->SetFogDensity(CurrentState.FogDensity);
        HeightFog->SetFogInscatteringColor(CurrentState.FogColor);
    }
    if (WarmBenchLamp)
    {
        WarmBenchLamp->SetIntensity(CurrentState.LampIntensity);
        WarmBenchLamp->SetLightColor(CurrentState.LampColor);
    }
    if (CampfireLight)
    {
        CampfireLight->SetIntensity(CurrentState.CampfireIntensity);
    }
}
