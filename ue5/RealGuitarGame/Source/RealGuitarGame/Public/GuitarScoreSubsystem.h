#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GuitarPerformanceTypes.h"
#include "GuitarScoreSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGuitarEventTriggered, const FGuitarEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGuitarChordChanged, const FGuitarChordSnapshot&, Chord);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGuitarScoreLoaded, const FGuitarPerformanceData&, Performance);

/**
 * World subsystem that loads canonical GuitarPerformance.json scores and dispatches
 * sample-accurate note/chord events for audio synthesis and Control Rig IK animation.
 */
UCLASS()
class REALGUITARGAME_API UGuitarScoreSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintCallable, Category = "Guitar|Score")
    bool LoadPerformanceFromJsonFile(const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Score")
    void Play();

    UFUNCTION(BlueprintCallable, Category = "Guitar|Score")
    void Pause();

    UFUNCTION(BlueprintCallable, Category = "Guitar|Score")
    void Stop();

    UFUNCTION(BlueprintCallable, Category = "Guitar|Score")
    void SeekTo(float TimeSec);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Score")
    void SetLooping(bool bInLoop) { bLooping = bInLoop; }

    UFUNCTION(BlueprintCallable, Category = "Guitar|Score")
    void SetPlaybackRate(float InRate) { PlaybackRate = FMath::Clamp(InRate, 0.25f, 2.5f); }

    UFUNCTION(BlueprintPure, Category = "Guitar|Score")
    bool IsPlaying() const { return bIsPlaying; }

    UFUNCTION(BlueprintPure, Category = "Guitar|Score")
    float GetCurrentTimeSec() const { return CurrentTimeSec; }

    UFUNCTION(BlueprintPure, Category = "Guitar|Score")
    const FGuitarPerformanceData& GetLoadedPerformance() const { return LoadedPerformance; }

    UFUNCTION(BlueprintPure, Category = "Guitar|Score")
    void GetActiveEvents(TArray<FGuitarEvent>& OutActiveEvents) const;

    UFUNCTION(BlueprintPure, Category = "Guitar|Score")
    bool GetCurrentChord(FGuitarChordSnapshot& OutChord) const;

    UPROPERTY(BlueprintAssignable, Category = "Guitar|Events")
    FOnGuitarEventTriggered OnGuitarEventTriggered;

    UPROPERTY(BlueprintAssignable, Category = "Guitar|Events")
    FOnGuitarChordChanged OnChordChanged;

    UPROPERTY(BlueprintAssignable, Category = "Guitar|Events")
    FOnGuitarScoreLoaded OnScoreLoaded;

private:
    static EGuitarRightAction ParseRightAction(const FString& Value);
    static EGuitarArticulation ParseArticulation(const FString& Value);

    FGuitarPerformanceData LoadedPerformance;
    bool bHasScore = false;
    bool bIsPlaying = false;
    bool bLooping = true;
    float PlaybackRate = 1.0f;
    float CurrentTimeSec = 0.0f;
    int32 NextEventIndex = 0;
    int32 CurrentChordIndex = INDEX_NONE;
};
