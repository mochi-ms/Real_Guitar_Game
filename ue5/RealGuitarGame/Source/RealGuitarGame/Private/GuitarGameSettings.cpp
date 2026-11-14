#include "GuitarGameSettings.h"

#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    EWindowMode::Type ToUnrealWindowMode(int32 ModeIndex)
    {
        switch (ModeIndex)
        {
        case 0:
            return EWindowMode::Fullscreen;
        case 2:
            return EWindowMode::Windowed;
        default:
            return EWindowMode::WindowedFullscreen;
        }
    }

    int32 FromUnrealWindowMode(EWindowMode::Type Mode)
    {
        switch (Mode)
        {
        case EWindowMode::Fullscreen:
            return 0;
        case EWindowMode::Windowed:
            return 2;
        default:
            return 1;
        }
    }

    ESanctuaryMood ParseMoodString(const FString& Name)
    {
        if (Name.Equals(TEXT("CozyNightLamp"), ESearchCase::IgnoreCase)) return ESanctuaryMood::CozyNightLamp;
        if (Name.Equals(TEXT("MorningMist"), ESearchCase::IgnoreCase)) return ESanctuaryMood::MorningMist;
        if (Name.Equals(TEXT("RainyLoFi"), ESearchCase::IgnoreCase)) return ESanctuaryMood::RainyLoFi;
        return ESanctuaryMood::GoldenSunset;
    }

    FString MoodToString(ESanctuaryMood Mood)
    {
        switch (Mood)
        {
        case ESanctuaryMood::CozyNightLamp:
            return TEXT("CozyNightLamp");
        case ESanctuaryMood::MorningMist:
            return TEXT("MorningMist");
        case ESanctuaryMood::RainyLoFi:
            return TEXT("RainyLoFi");
        default:
            return TEXT("GoldenSunset");
        }
    }
}

void UGuitarGameSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (GEngine)
    {
        if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
        {
            UserSettings->LoadSettings(false);
            const FIntPoint Desktop = UserSettings->GetDesktopResolution();
            if (Desktop.X >= 1280 && Desktop.Y >= 720)
            {
                CurrentSettings.Resolution = Desktop;
            }
            CurrentSettings.WindowMode = FromUnrealWindowMode(UserSettings->GetFullscreenMode());
            CurrentSettings.bVSync = UserSettings->IsVSyncEnabled();
            CurrentSettings.FrameRateLimit = UserSettings->GetFrameRateLimit();
        }
    }

    LoadFromDisk();
    ClampSettings(CurrentSettings);
    ApplyEngineAndAudioSettings(false);
}

void UGuitarGameSettingsSubsystem::UpdateAndApplySettings(const FGuitarSettingsData& NewSettings, bool bSaveToDisk)
{
    CurrentSettings = NewSettings;
    ClampSettings(CurrentSettings);
    ApplyEngineAndAudioSettings(bSaveToDisk);

    if (bSaveToDisk)
    {
        SaveToDisk();
    }

    OnSettingsChanged.Broadcast(CurrentSettings);
}

void UGuitarGameSettingsSubsystem::ResetToDefaults()
{
    FGuitarSettingsData Defaults;
    if (GEngine)
    {
        if (const UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
        {
            const FIntPoint Desktop = UserSettings->GetDesktopResolution();
            if (Desktop.X >= 1280 && Desktop.Y >= 720)
            {
                Defaults.Resolution = Desktop;
            }
        }
    }
    UpdateAndApplySettings(Defaults, true);
}

void UGuitarGameSettingsSubsystem::SetAtmosphereMood(ESanctuaryMood NewMood, bool bSaveToDisk)
{
    if (CurrentSettings.AtmosphereMood == NewMood)
    {
        return;
    }
    CurrentSettings.AtmosphereMood = NewMood;
    if (bSaveToDisk)
    {
        SaveToDisk();
    }
    OnSettingsChanged.Broadcast(CurrentSettings);
}

ESanctuaryMood UGuitarGameSettingsSubsystem::CycleAtmosphereMood()
{
    const uint8 NextIndex = (static_cast<uint8>(CurrentSettings.AtmosphereMood) + 1) % 4;
    const ESanctuaryMood NextMood = static_cast<ESanctuaryMood>(NextIndex);
    SetAtmosphereMood(NextMood, true);
    return NextMood;
}

FText UGuitarGameSettingsSubsystem::GetMoodDisplayName(ESanctuaryMood Mood)
{
    switch (Mood)
    {
    case ESanctuaryMood::CozyNightLamp:
        return NSLOCTEXT("GuitarMood", "Night", "아늑한 밤 램프 (Night)");
    case ESanctuaryMood::MorningMist:
        return NSLOCTEXT("GuitarMood", "Morning", "아침 물안개 (Morning)");
    case ESanctuaryMood::RainyLoFi:
        return NSLOCTEXT("GuitarMood", "Rain", "비 오는 로파이 (Rainy)");
    default:
        return NSLOCTEXT("GuitarMood", "Sunset", "황금빛 노을 (Sunset)");
    }
}

FText UGuitarGameSettingsSubsystem::GetGraphicsPresetName(int32 PresetIndex)
{
    switch (FMath::Clamp(PresetIndex, 0, 4))
    {
    case 0:
        return NSLOCTEXT("GuitarSettings", "Low", "낮음 (Low)");
    case 1:
        return NSLOCTEXT("GuitarSettings", "Medium", "보통 (Medium)");
    case 2:
        return NSLOCTEXT("GuitarSettings", "High", "높음 (High)");
    case 4:
        return NSLOCTEXT("GuitarSettings", "Cinematic", "시네마틱 (Cinematic)");
    default:
        return NSLOCTEXT("GuitarSettings", "Epic", "에픽 (Epic)");
    }
}

TArray<FIntPoint> UGuitarGameSettingsSubsystem::GetSupportedResolutions()
{
    TArray<FIntPoint> List = {
        FIntPoint(1280, 720),
        FIntPoint(1600, 900),
        FIntPoint(1920, 1080),
        FIntPoint(2560, 1440),
        FIntPoint(3840, 2160)
    };
    if (GEngine)
    {
        if (const UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
        {
            const FIntPoint Desktop = UserSettings->GetDesktopResolution();
            if (Desktop.X >= 1280 && Desktop.Y >= 720 && !List.Contains(Desktop))
            {
                List.Add(Desktop);
            }
        }
    }
    return List;
}

FString UGuitarGameSettingsSubsystem::GetSettingsFilePath() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Config"), TEXT("RealGuitarSettings.json"));
}

void UGuitarGameSettingsSubsystem::ClampSettings(FGuitarSettingsData& InOutSettings)
{
    InOutSettings.Resolution.X = FMath::Clamp(InOutSettings.Resolution.X, 1024, 7680);
    InOutSettings.Resolution.Y = FMath::Clamp(InOutSettings.Resolution.Y, 640, 4320);
    InOutSettings.WindowMode = FMath::Clamp(InOutSettings.WindowMode, 0, 2);
    InOutSettings.FrameRateLimit = (InOutSettings.FrameRateLimit <= 0.0f)
        ? 0.0f
        : FMath::Clamp(InOutSettings.FrameRateLimit, 30.0f, 240.0f);
    InOutSettings.GraphicsPreset = FMath::Clamp(InOutSettings.GraphicsPreset, 0, 4);
    InOutSettings.MasterVolume = FMath::Clamp(InOutSettings.MasterVolume, 0.0f, 1.0f);
    InOutSettings.GuitarVolume = FMath::Clamp(InOutSettings.GuitarVolume, 0.0f, 1.0f);
    InOutSettings.AmbientVolume = FMath::Clamp(InOutSettings.AmbientVolume, 0.0f, 1.0f);
    InOutSettings.ReverbMix = FMath::Clamp(InOutSettings.ReverbMix, 0.0f, 1.0f);
    InOutSettings.MouseSensitivity = FMath::Clamp(InOutSettings.MouseSensitivity, 0.2f, 3.0f);
    InOutSettings.FieldOfView = FMath::Clamp(InOutSettings.FieldOfView, 55.0f, 110.0f);
}

void UGuitarGameSettingsSubsystem::ApplyEngineAndAudioSettings(bool bSaveEngineConfig)
{
    if (!GEngine)
    {
        return;
    }

    if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
    {
        UserSettings->SetScreenResolution(CurrentSettings.Resolution);
        UserSettings->SetFullscreenMode(ToUnrealWindowMode(CurrentSettings.WindowMode));
        UserSettings->SetVSyncEnabled(CurrentSettings.bVSync);
        UserSettings->SetFrameRateLimit(CurrentSettings.FrameRateLimit);
        UserSettings->SetOverallScalabilityLevel(CurrentSettings.GraphicsPreset);

        if (CurrentSettings.bLumenEnabled)
        {
            const int32 LumenTier = FMath::Max(2, CurrentSettings.GraphicsPreset);
            UserSettings->SetGlobalIlluminationQuality(LumenTier);
            UserSettings->SetReflectionQuality(LumenTier);
        }
        else
        {
            UserSettings->SetGlobalIlluminationQuality(0);
            UserSettings->SetReflectionQuality(1);
        }

        UserSettings->ApplySettings(false);
        if (bSaveEngineConfig)
        {
            UserSettings->SaveSettings();
        }
    }
}

bool UGuitarGameSettingsSubsystem::LoadFromDisk()
{
    FString JsonText;
    const FString FilePath = GetSettingsFilePath();
    if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
    {
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return false;
    }

    int32 IntVal = 0;
    double NumVal = 0.0;
    bool BoolVal = false;
    FString StrVal;

    if (Root->TryGetNumberField(TEXT("res_x"), IntVal)) CurrentSettings.Resolution.X = IntVal;
    if (Root->TryGetNumberField(TEXT("res_y"), IntVal)) CurrentSettings.Resolution.Y = IntVal;
    if (Root->TryGetNumberField(TEXT("window_mode"), IntVal)) CurrentSettings.WindowMode = IntVal;
    if (Root->TryGetNumberField(TEXT("fps_limit"), NumVal)) CurrentSettings.FrameRateLimit = static_cast<float>(NumVal);
    if (Root->TryGetBoolField(TEXT("vsync"), BoolVal)) CurrentSettings.bVSync = BoolVal;
    if (Root->TryGetNumberField(TEXT("graphics_preset"), IntVal)) CurrentSettings.GraphicsPreset = IntVal;
    if (Root->TryGetBoolField(TEXT("lumen_enabled"), BoolVal)) CurrentSettings.bLumenEnabled = BoolVal;

    if (Root->TryGetNumberField(TEXT("master_volume"), NumVal)) CurrentSettings.MasterVolume = static_cast<float>(NumVal);
    if (Root->TryGetNumberField(TEXT("guitar_volume"), NumVal)) CurrentSettings.GuitarVolume = static_cast<float>(NumVal);
    if (Root->TryGetNumberField(TEXT("ambient_volume"), NumVal)) CurrentSettings.AmbientVolume = static_cast<float>(NumVal);
    if (Root->TryGetNumberField(TEXT("reverb_mix"), NumVal)) CurrentSettings.ReverbMix = static_cast<float>(NumVal);

    if (Root->TryGetNumberField(TEXT("mouse_sensitivity"), NumVal)) CurrentSettings.MouseSensitivity = static_cast<float>(NumVal);
    if (Root->TryGetBoolField(TEXT("invert_y"), BoolVal)) CurrentSettings.bInvertY = BoolVal;
    if (Root->TryGetNumberField(TEXT("fov"), NumVal)) CurrentSettings.FieldOfView = static_cast<float>(NumVal);
    if (Root->TryGetBoolField(TEXT("auto_hide_hud_when_seated"), BoolVal)) CurrentSettings.bAutoHideHudWhenSeated = BoolVal;
    if (Root->TryGetBoolField(TEXT("show_chord_diagrams"), BoolVal)) CurrentSettings.bShowChordDiagrams = BoolVal;
    if (Root->TryGetStringField(TEXT("atmosphere_mood"), StrVal)) CurrentSettings.AtmosphereMood = ParseMoodString(StrVal);

    return true;
}

bool UGuitarGameSettingsSubsystem::SaveToDisk() const
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(TEXT("res_x"), CurrentSettings.Resolution.X);
    Root->SetNumberField(TEXT("res_y"), CurrentSettings.Resolution.Y);
    Root->SetNumberField(TEXT("window_mode"), CurrentSettings.WindowMode);
    Root->SetNumberField(TEXT("fps_limit"), CurrentSettings.FrameRateLimit);
    Root->SetBoolField(TEXT("vsync"), CurrentSettings.bVSync);
    Root->SetNumberField(TEXT("graphics_preset"), CurrentSettings.GraphicsPreset);
    Root->SetBoolField(TEXT("lumen_enabled"), CurrentSettings.bLumenEnabled);

    Root->SetNumberField(TEXT("master_volume"), CurrentSettings.MasterVolume);
    Root->SetNumberField(TEXT("guitar_volume"), CurrentSettings.GuitarVolume);
    Root->SetNumberField(TEXT("ambient_volume"), CurrentSettings.AmbientVolume);
    Root->SetNumberField(TEXT("reverb_mix"), CurrentSettings.ReverbMix);

    Root->SetNumberField(TEXT("mouse_sensitivity"), CurrentSettings.MouseSensitivity);
    Root->SetBoolField(TEXT("invert_y"), CurrentSettings.bInvertY);
    Root->SetNumberField(TEXT("fov"), CurrentSettings.FieldOfView);
    Root->SetBoolField(TEXT("auto_hide_hud_when_seated"), CurrentSettings.bAutoHideHudWhenSeated);
    Root->SetBoolField(TEXT("show_chord_diagrams"), CurrentSettings.bShowChordDiagrams);
    Root->SetStringField(TEXT("atmosphere_mood"), MoodToString(CurrentSettings.AtmosphereMood));

    FString OutJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    if (!FJsonSerializer::Serialize(Root, Writer))
    {
        return false;
    }

    const FString FilePath = GetSettingsFilePath();
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*FPaths::GetPath(FilePath));
    return FFileHelper::SaveStringToFile(OutJson, *FilePath);
}
