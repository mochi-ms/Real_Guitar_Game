#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GuitarPerformanceTypes.h"
#include "GuitarSongLibrarySubsystem.generated.h"

USTRUCT(BlueprintType)
struct REALGUITARGAME_API FGuitarSongLibraryEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Library")
    FString DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Library")
    FString FilePath;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Library")
    bool bIsUserScore = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSongLibraryUpdated, const TArray<FGuitarSongLibraryEntry>&, Songs);

/**
 * Manages available compiled .performance.json scores and provides an in-game
 * live Chord + Strum/Fingerstyle pattern generator for immediate experimentation.
 */
UCLASS()
class REALGUITARGAME_API UGuitarSongLibrarySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Guitar|Library")
    void RefreshSongList();

    UFUNCTION(BlueprintPure, Category = "Guitar|Library")
    const TArray<FGuitarSongLibraryEntry>& GetAvailableSongs() const { return Songs; }

    UFUNCTION(BlueprintPure, Category = "Guitar|Library")
    int32 GetCurrentSongIndex() const { return CurrentIndex; }

    UFUNCTION(BlueprintCallable, Category = "Guitar|Library")
    bool LoadSongByIndex(int32 Index, bool bAutoPlay = true);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Library")
    bool PlayNextSong();

    UFUNCTION(BlueprintCallable, Category = "Guitar|Library")
    bool PlayPreviousSong();

    UFUNCTION(BlueprintCallable, Category = "Guitar|Library")
    void GenerateAndPlayCustomChart(
        const TArray<FString>& ChordSymbols,
        float Bpm,
        bool bFingerstyle,
        const FString& PresetName,
        bool bAutoPlay = true
    );

    UFUNCTION(BlueprintPure, Category = "Guitar|Library")
    static bool ResolveChordShape(const FString& Symbol, TArray<int32>& OutFrets, TArray<int32>& OutFingers);

    UFUNCTION(BlueprintPure, Category = "Guitar|Library")
    static int32 ComputeMidiNote(int32 StringIndex, int32 Fret);

    UFUNCTION(BlueprintPure, Category = "Guitar|Library")
    static int32 GetNumChordBanks();

    UFUNCTION(BlueprintPure, Category = "Guitar|Library")
    static FString GetChordBankName(int32 BankIndex);

    UFUNCTION(BlueprintPure, Category = "Guitar|Library")
    static TArray<FString> GetChordBankSymbols(int32 BankIndex);

    UPROPERTY(BlueprintAssignable, Category = "Guitar|Library")
    FOnSongLibraryUpdated OnSongLibraryUpdated;

private:
    TArray<FGuitarSongLibraryEntry> Songs;
    int32 CurrentIndex = INDEX_NONE;
};
