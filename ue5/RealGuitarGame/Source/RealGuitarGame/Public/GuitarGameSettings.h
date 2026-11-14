#pragma once

#include "CoreMinimal.h"
#include "GuitarPerformanceTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GuitarGameSettings.generated.h"

USTRUCT(BlueprintType)
struct REALGUITARGAME_API FGuitarSettingsData
{
    GENERATED_BODY()

    // Display & Graphics
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Display")
    FIntPoint Resolution = FIntPoint(1920, 1080);

    // 0 = Fullscreen, 1 = WindowedFullscreen (Borderless), 2 = Windowed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Display")
    int32 WindowMode = 1;

    // 0 = Unlimited, otherwise 30 / 60 / 120 / 144
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Display")
    float FrameRateLimit = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Display")
    bool bVSync = true;

    // 0 = Low, 1 = Medium, 2 = High, 3 = Epic, 4 = Cinematic
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
    int32 GraphicsPreset = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
    bool bLumenEnabled = true;

    // Audio Mix
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
    float MasterVolume = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
    float GuitarVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
    float AmbientVolume = 0.60f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
    float ReverbMix = 0.35f;

    // Controls & UX
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Controls")
    float MouseSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Controls")
    bool bInvertY = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Controls")
    float FieldOfView = 75.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|UX")
    bool bAutoHideHudWhenSeated = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|UX")
    bool bShowChordDiagrams = true;

    // Atmosphere
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Atmosphere")
    ESanctuaryMood AtmosphereMood = ESanctuaryMood::GoldenSunset;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGuitarSettingsChanged, const FGuitarSettingsData&, NewSettings);

/**
 * Manages persisted user settings (Graphics, Display, Audio mix, Controls, Sanctuary Mood),
 * syncing display/scalability values directly with UGameUserSettings and saving JSON to Saved/Config.
 */
UCLASS()
class REALGUITARGAME_API UGuitarGameSettingsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintPure, Category = "Guitar|Settings")
    const FGuitarSettingsData& GetSettings() const { return CurrentSettings; }

    UFUNCTION(BlueprintCallable, Category = "Guitar|Settings")
    void UpdateAndApplySettings(const FGuitarSettingsData& NewSettings, bool bSaveToDisk = true);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Settings")
    void ResetToDefaults();

    UFUNCTION(BlueprintCallable, Category = "Guitar|Settings")
    void SetAtmosphereMood(ESanctuaryMood NewMood, bool bSaveToDisk = true);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Settings")
    ESanctuaryMood CycleAtmosphereMood();

    UFUNCTION(BlueprintPure, Category = "Guitar|Settings")
    static FText GetMoodDisplayName(ESanctuaryMood Mood);

    UFUNCTION(BlueprintPure, Category = "Guitar|Settings")
    static FText GetGraphicsPresetName(int32 PresetIndex);

    UFUNCTION(BlueprintPure, Category = "Guitar|Settings")
    static TArray<FIntPoint> GetSupportedResolutions();

    UPROPERTY(BlueprintAssignable, Category = "Guitar|Settings")
    FOnGuitarSettingsChanged OnSettingsChanged;

private:
    FString GetSettingsFilePath() const;
    bool LoadFromDisk();
    bool SaveToDisk() const;
    void ApplyEngineAndAudioSettings(bool bSaveEngineConfig);
    static void ClampSettings(FGuitarSettingsData& InOutSettings);

    FGuitarSettingsData CurrentSettings;
};
