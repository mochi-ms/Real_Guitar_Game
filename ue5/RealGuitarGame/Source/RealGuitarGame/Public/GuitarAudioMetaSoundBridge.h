#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GuitarPerformanceTypes.h"
#include "GuitarAudioMetaSoundBridge.generated.h"

struct FGuitarSettingsData;
class UAudioComponent;
class USoundBase;

/**
 * Bridges UGuitarScoreSubsystem note/strum events into either a UE5 MetaSound / Harmonix asset
 * OR, when GuitarMetaSoundAsset is unset, directly into a polyphonic multi-sample voice pool
 * loaded from Content/Audio/multisample_map.json + /Game/Audio/Samples.
 */
UCLASS(ClassGroup = (Guitar), meta = (BlueprintSpawnableComponent))
class REALGUITARGAME_API UGuitarAudioMetaSoundBridge : public UActorComponent
{
    GENERATED_BODY()

public:
    UGuitarAudioMetaSoundBridge();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guitar|Audio")
    TObjectPtr<USoundBase> GuitarMetaSoundAsset;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guitar|Audio")
    TObjectPtr<UAudioComponent> AudioComponent;

    UFUNCTION(BlueprintCallable, Category = "Guitar|Audio")
    void EnsureAudioStarted();

    UFUNCTION(BlueprintCallable, Category = "Guitar|Audio")
    void StopAllNotes();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    using FSampleZone = FGuitarSampleZone;

    UFUNCTION()
    void HandleGuitarEvent(const FGuitarEvent& Event);

    UFUNCTION()
    void HandleSettingsChanged(const FGuitarSettingsData& NewSettings);

    void LoadSampleZonesFromJson();
    void EnsureVoicePoolCreated();
    void PlayMultiSampleNote(const FGuitarEvent& Event);
    void ApplyAudioMixFromSettings();
    const FSampleZone* FindBestZone(int32 MidiNote, float Velocity01) const;
    static USoundBase* LoadSampleAssetFromRelativeWavPath(const FString& RelWavPath);

    static constexpr int32 NumPoolVoices = 24;

    UPROPERTY(Transient)
    TArray<FGuitarSampleZone> SampleZones;
    UPROPERTY(Transient)
    TObjectPtr<USoundBase> PercussiveChuckSound;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UAudioComponent>> VoicePool;

    int32 NextVoiceCursor = 0;
    int32 StringActiveVoiceIdx[6] = { -1, -1, -1, -1, -1, -1 };
};
