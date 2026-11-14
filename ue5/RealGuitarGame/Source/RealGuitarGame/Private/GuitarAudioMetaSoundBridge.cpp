#include "GuitarAudioMetaSoundBridge.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
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
    }
}

void UGuitarAudioMetaSoundBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->OnGuitarEventTriggered.RemoveDynamic(this, &UGuitarAudioMetaSoundBridge::HandleGuitarEvent);
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
