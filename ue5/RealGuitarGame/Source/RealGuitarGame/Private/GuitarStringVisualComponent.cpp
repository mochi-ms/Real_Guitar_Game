#include "GuitarStringVisualComponent.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GuitarScoreSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"

UGuitarStringVisualComponent::UGuitarStringVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    StringAmplitudes.Init(0.0f, 6);
    StringDecayRates.Init(4.2f, 6);
    ActiveFrets.Init(0, 6);
}

void UGuitarStringVisualComponent::BeginPlay()
{
    Super::BeginPlay();

    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->OnGuitarEventTriggered.AddDynamic(this, &UGuitarStringVisualComponent::HandleGuitarEvent);
        }
    }
}

void UGuitarStringVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->OnGuitarEventTriggered.RemoveDynamic(this, &UGuitarStringVisualComponent::HandleGuitarEvent);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void UGuitarStringVisualComponent::RegisterStringMesh(UMeshComponent* TargetMesh, int32 MaterialSlotIndex)
{
    if (!TargetMesh)
    {
        return;
    }
    if (UMaterialInstanceDynamic* MID = TargetMesh->CreateAndSetMaterialInstanceDynamic(MaterialSlotIndex))
    {
        DynamicMaterials.AddUnique(MID);
    }
}

float UGuitarStringVisualComponent::GetStringAmplitude(int32 StringIndex) const
{
    const int32 Idx = StringIndex - 1;
    return StringAmplitudes.IsValidIndex(Idx) ? StringAmplitudes[Idx] : 0.0f;
}

void UGuitarStringVisualComponent::HandleGuitarEvent(const FGuitarEvent& Event)
{
    const int32 Idx = Event.StringIndex - 1;
    if (!StringAmplitudes.IsValidIndex(Idx))
    {
        return;
    }

    const bool bMuted =
        Event.Articulation == EGuitarArticulation::PalmMute ||
        Event.Articulation == EGuitarArticulation::DeadNote;

    StringAmplitudes[Idx] = bMuted ? Event.Velocity * 0.45f : FMath::Clamp(Event.Velocity, 0.25f, 1.0f);
    StringDecayRates[Idx] = bMuted ? PalmMuteDecayRate : VibrationDecayRate;
    ActiveFrets[Idx] = Event.Fret;
}

void UGuitarStringVisualComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    static const FName VibParams[6] = {
        TEXT("StringVibration_1"),
        TEXT("StringVibration_2"),
        TEXT("StringVibration_3"),
        TEXT("StringVibration_4"),
        TEXT("StringVibration_5"),
        TEXT("StringVibration_6")
    };
    static const FName FretParams[6] = {
        TEXT("ActiveFret_1"),
        TEXT("ActiveFret_2"),
        TEXT("ActiveFret_3"),
        TEXT("ActiveFret_4"),
        TEXT("ActiveFret_5"),
        TEXT("ActiveFret_6")
    };

    for (int32 i = 0; i < 6; ++i)
    {
        StringAmplitudes[i] *= FMath::Exp(-StringDecayRates[i] * DeltaTime);
        if (StringAmplitudes[i] < 0.005f)
        {
            StringAmplitudes[i] = 0.0f;
        }
    }

    for (UMaterialInstanceDynamic* MID : DynamicMaterials)
    {
        if (!MID)
        {
            continue;
        }
        for (int32 i = 0; i < 6; ++i)
        {
            MID->SetScalarParameterValue(VibParams[i], StringAmplitudes[i]);
            MID->SetScalarParameterValue(FretParams[i], static_cast<float>(ActiveFrets[i]));
        }
    }
}
