#include "GuitarAudioMetaSoundBridge.h"

#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GuitarGameSettings.h"
#include "GuitarScoreSubsystem.h"

UGuitarAudioMetaSoundBridge::UGuitarAudioMetaSoundBridge()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGuitarAudioMetaSoundBridge::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (Owner && !AudioComponent)
    {
        AudioComponent = NewObject<UAudioComponent>(Owner, TEXT("GuitarMetaSoundAudio"));
        if (AudioComponent)
        {
            AudioComponent->bAutoActivate = false;
            AudioComponent->bAllowSpatialization = true;
            AudioComponent->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
            AudioComponent->RegisterComponent();
        }
    }

    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->OnGuitarEventTriggered.AddDynamic(this, &UGuitarAudioMetaSoundBridge::HandleGuitarEvent);
        }
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
            {
                SettingsSub->OnSettingsChanged.AddDynamic(this, &UGuitarAudioMetaSoundBridge::HandleSettingsChanged);
            }
        }
    }
    ApplyAudioMixFromSettings();
}

void UGuitarAudioMetaSoundBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->OnGuitarEventTriggered.RemoveDynamic(this, &UGuitarAudioMetaSoundBridge::HandleGuitarEvent);
        }
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
            {
                SettingsSub->OnSettingsChanged.RemoveDynamic(this, &UGuitarAudioMetaSoundBridge::HandleSettingsChanged);
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}

void UGuitarAudioMetaSoundBridge::EnsureAudioStarted()
{
    if (!AudioComponent || !GuitarMetaSoundAsset)
    {
        return;
    }
    if (AudioComponent->GetSound() != GuitarMetaSoundAsset)
    {
        AudioComponent->SetSound(GuitarMetaSoundAsset);
    }
    if (!AudioComponent->IsPlaying())
    {
        AudioComponent->Play();
    }
    ApplyAudioMixFromSettings();
}

void UGuitarAudioMetaSoundBridge::HandleSettingsChanged(const FGuitarSettingsData& NewSettings)
{
    ApplyAudioMixFromSettings();
}

void UGuitarAudioMetaSoundBridge::ApplyAudioMixFromSettings()
{
    if (!AudioComponent)
    {
        return;
    }
    const UWorld* World = GetWorld();
    const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    const UGuitarGameSettingsSubsystem* SettingsSub = GI ? GI->GetSubsystem<UGuitarGameSettingsSubsystem>() : nullptr;
    if (!SettingsSub)
    {
        return;
    }

    const FGuitarSettingsData& S = SettingsSub->GetSettings();
    const float CombinedVolume = FMath::Clamp(S.MasterVolume * S.GuitarVolume, 0.0f, 1.0f);
    AudioComponent->SetVolumeMultiplier(CombinedVolume);
    AudioComponent->SetFloatParameter(TEXT("MasterVolume"), S.MasterVolume);
    AudioComponent->SetFloatParameter(TEXT("GuitarVolume"), S.GuitarVolume);
    AudioComponent->SetFloatParameter(TEXT("ReverbMix"), S.ReverbMix);
}

void UGuitarAudioMetaSoundBridge::HandleGuitarEvent(const FGuitarEvent& Event)
{
    EnsureAudioStarted();
    if (!AudioComponent || !AudioComponent->IsPlaying())
    {
        return;
    }

    AudioComponent->SetIntParameter(TEXT("MidiNote"), Event.MidiNote);
    AudioComponent->SetIntParameter(TEXT("StringIndex"), Event.StringIndex);
    AudioComponent->SetIntParameter(TEXT("Fret"), Event.Fret);
    AudioComponent->SetFloatParameter(TEXT("Velocity"), Event.Velocity);
    AudioComponent->SetFloatParameter(TEXT("DurationSec"), Event.DurationSec);
    AudioComponent->SetIntParameter(TEXT("Articulation"), static_cast<int32>(Event.Articulation));
    AudioComponent->SetBoolParameter(
        TEXT("IsDownStrum"),
        Event.RightAction == EGuitarRightAction::StrumDown || Event.RightAction == EGuitarRightAction::PickDown
    );
    AudioComponent->SetTriggerParameter(TEXT("TriggerPluck"));
}
