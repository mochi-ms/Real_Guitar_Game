#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GuitarPerformanceTypes.h"
#include "GuitarAudioMetaSoundBridge.generated.h"

struct FGuitarSettingsData;
class UAudioComponent;
class USoundBase;

/**
 * Bridges UGuitarScoreSubsystem note/strum events into a UE5 MetaSound / Harmonix Fusion Sampler
 * AudioComponent via named audio parameters.
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

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void HandleGuitarEvent(const FGuitarEvent& Event);

    UFUNCTION()
    void HandleSettingsChanged(const FGuitarSettingsData& NewSettings);

    void ApplyAudioMixFromSettings();
};
