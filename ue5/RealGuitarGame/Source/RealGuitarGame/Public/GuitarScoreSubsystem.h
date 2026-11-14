#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GuitarPerformanceTypes.h"
#include "GuitarScoreSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGuitarEventTriggered, const FGuitarEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGuitarChordChanged, const FGuitarChordSnapshot&, Chord);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGuitarScoreLoaded, const FGuitarPerformanceData&, Performance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayControlModeChanged, EPlayControlMode, NewMode);

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
    void LoadPerformanceData(const FGuitarPerformanceData& InData);

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
    bool IsLooping() const { return bLooping; }

    UFUNCTION(BlueprintPure, Category = "Guitar|Score")
    float GetPlaybackRate() const { return PlaybackRate; }

    UFUNCTION(BlueprintPure, Category = "Guitar|Score")
    float GetCurrentTimeSec() const { return CurrentTimeSec; }

    UFUNCTION(BlueprintPure, Category = "Guitar|Score")
    const FGuitarPerformanceData& GetLoadedPerformance() const { return LoadedPerformance; }

    UFUNCTION(BlueprintPure, Category = "Guitar|Score")
    void GetActiveEvents(TArray<FGuitarEvent>& OutActiveEvents) const;

    UFUNCTION(BlueprintPure, Category = "Guitar|Score")
    bool GetCurrentChord(FGuitarChordSnapshot& OutChord) const;

    UFUNCTION(BlueprintPure, Category = "Guitar|Score")
    bool GetNextChord(FGuitarChordSnapshot& OutChord) const;

    // Live Manual Guitar Performance API
    UFUNCTION(BlueprintCallable, Category = "Guitar|Live")
    void SetControlMode(EPlayControlMode NewMode);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Live")
    void ToggleControlMode();

    UFUNCTION(BlueprintPure, Category = "Guitar|Live")
    EPlayControlMode GetControlMode() const { return ControlMode; }

    UFUNCTION(BlueprintCallable, Category = "Guitar|Live")
    void SetChordBankIndex(int32 BankIndex);

    UFUNCTION(BlueprintPure, Category = "Guitar|Live")
    int32 GetChordBankIndex() const { return ChordBankIndex; }

    UFUNCTION(BlueprintPure, Category = "Guitar|Live")
    TArray<FString> GetLiveChordSlots();

    UFUNCTION(BlueprintPure, Category = "Guitar|Live")
    int32 GetActiveLiveChordSlot() const { return ActiveLiveChordSlot; }

    UFUNCTION(BlueprintCallable, Category = "Guitar|Live")
    void SelectLiveChordSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Live")
    void SelectLiveChordSymbol(const FString& Symbol);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Live")
    void TriggerLiveStrum(bool bDownStrum, float Velocity = 0.85f);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Live")
    void PluckLiveString(int32 String1To6, float Velocity = 0.85f);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Live")
    void TriggerLivePercussive(float Velocity = 0.72f);

    UPROPERTY(BlueprintAssignable, Category = "Guitar|Events")
    FOnGuitarEventTriggered OnGuitarEventTriggered;

    UPROPERTY(BlueprintAssignable, Category = "Guitar|Events")
    FOnGuitarChordChanged OnChordChanged;

    UPROPERTY(BlueprintAssignable, Category = "Guitar|Events")
    FOnGuitarScoreLoaded OnScoreLoaded;

    UPROPERTY(BlueprintAssignable, Category = "Guitar|Events")
    FOnPlayControlModeChanged OnControlModeChanged;

private:
    void ProcessLiveEventsTick(float DeltaTime);
    void EmitLiveNoteNow(const FGuitarEvent& Ev);
    void EnsureLiveBankInitialized();
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

    EPlayControlMode ControlMode = EPlayControlMode::FreePlay;
    int32 ChordBankIndex = 0;
    TArray<FString> LiveChordSlots;
    int32 ActiveLiveChordSlot = 0;
    FGuitarChordSnapshot CurrentLiveChord;
    float LiveClockSec = 0.0f;
    int32 LiveEventCounter = 100000;
    TArray<FGuitarEvent> PendingLiveEvents;
    TArray<FGuitarEvent> RingingLiveEvents;
};
