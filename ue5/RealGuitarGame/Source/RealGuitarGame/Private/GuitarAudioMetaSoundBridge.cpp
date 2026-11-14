#include "GuitarAudioMetaSoundBridge.h"

#include "Components/AudioComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GuitarGameSettings.h"
#include "GuitarScoreSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"

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

    EnsureVoicePoolCreated();
    if (!GuitarMetaSoundAsset)
    {
        LoadSampleZonesFromJson();
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
    StopAllNotes();

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

void UGuitarAudioMetaSoundBridge::EnsureVoicePoolCreated()
{
    AActor* Owner = GetOwner();
    if (!Owner || VoicePool.Num() > 0)
    {
        return;
    }

    VoicePool.Reserve(NumPoolVoices);
    for (int32 i = 0; i < NumPoolVoices; ++i)
    {
        const FName CompName(*FString::Printf(TEXT("GuitarVoice_%d"), i));
        UAudioComponent* Voice = NewObject<UAudioComponent>(Owner, CompName);
        if (Voice)
        {
            Voice->bAutoActivate = false;
            Voice->bAllowSpatialization = true;
            Voice->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
            Voice->RegisterComponent();
            VoicePool.Add(Voice);
        }
    }
}

USoundBase* UGuitarAudioMetaSoundBridge::LoadSampleAssetFromRelativeWavPath(const FString& RelWavPath)
{
    // "Samples/Acoustic_040_E2_soft.wav" -> "/Game/Audio/Samples/Acoustic_040_E2_soft.Acoustic_040_E2_soft"
    const FString BaseName = FPaths::GetBaseFilename(RelWavPath);
    if (BaseName.IsEmpty())
    {
        return nullptr;
    }
    const FString AssetPath = FString::Printf(TEXT("/Game/Audio/Samples/%s.%s"), *BaseName, *BaseName);
    return LoadObject<USoundBase>(nullptr, *AssetPath);
}

void UGuitarAudioMetaSoundBridge::LoadSampleZonesFromJson()
{
    SampleZones.Reset();
    const FString MapPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Audio"), TEXT("multisample_map.json"));
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *MapPath))
    {
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return;
    }

    FString ChuckPath;
    if (Root->TryGetStringField(TEXT("percussive_chuck"), ChuckPath))
    {
        PercussiveChuckSound = LoadSampleAssetFromRelativeWavPath(ChuckPath);
    }

    const TArray<TSharedPtr<FJsonValue>>* ZonesArr = nullptr;
    if (!Root->TryGetArrayField(TEXT("zones"), ZonesArr))
    {
        return;
    }

    for (const TSharedPtr<FJsonValue>& Val : *ZonesArr)
    {
        const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
        if (!Val->TryGetObject(ObjPtr) || !ObjPtr || !ObjPtr->IsValid())
        {
            continue;
        }
        const TSharedPtr<FJsonObject>& Obj = *ObjPtr;
        FString WavRel;
        if (!Obj->TryGetStringField(TEXT("wav_file"), WavRel))
        {
            continue;
        }
        USoundBase* LoadedSound = LoadSampleAssetFromRelativeWavPath(WavRel);
        if (!LoadedSound)
        {
            continue;
        }

        FSampleZone Zone;
        int32 IntVal = 0;
        if (Obj->TryGetNumberField(TEXT("root_midi"), IntVal)) Zone.RootMidi = IntVal;
        if (Obj->TryGetNumberField(TEXT("min_midi"), IntVal)) Zone.MinMidi = IntVal;
        if (Obj->TryGetNumberField(TEXT("max_midi"), IntVal)) Zone.MaxMidi = IntVal;
        if (Obj->TryGetNumberField(TEXT("min_velocity"), IntVal)) Zone.MinVel = IntVal;
        if (Obj->TryGetNumberField(TEXT("max_velocity"), IntVal)) Zone.MaxVel = IntVal;
        Zone.SoundAsset = LoadedSound;
        SampleZones.Add(Zone);
    }
}

const UGuitarAudioMetaSoundBridge::FSampleZone* UGuitarAudioMetaSoundBridge::FindBestZone(int32 MidiNote, float Velocity01) const
{
    if (SampleZones.IsEmpty())
    {
        return nullptr;
    }

    const int32 Vel127 = FMath::Clamp(FMath::RoundToInt(Velocity01 * 127.0f), 1, 127);
    const FSampleZone* BestMatch = nullptr;
    int32 BestScore = TNumericLimits<int32>::Max();

    for (const FSampleZone& Zone : SampleZones)
    {
        const int32 PitchDist = (MidiNote >= Zone.MinMidi && MidiNote <= Zone.MaxMidi)
            ? FMath::Abs(MidiNote - Zone.RootMidi)
            : 100 + FMath::Abs(MidiNote - Zone.RootMidi);
        const int32 VelPenalty = (Vel127 >= Zone.MinVel && Vel127 <= Zone.MaxVel) ? 0 : 25;
        const int32 Score = PitchDist + VelPenalty;
        if (Score < BestScore)
        {
            BestScore = Score;
            BestMatch = &Zone;
        }
    }
    return BestMatch;
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

void UGuitarAudioMetaSoundBridge::StopAllNotes()
{
    for (UAudioComponent* Voice : VoicePool)
    {
        if (Voice && Voice->IsPlaying())
        {
            Voice->Stop();
        }
    }
    for (int32& Idx : StringActiveVoiceIdx)
    {
        Idx = -1;
    }
}

void UGuitarAudioMetaSoundBridge::HandleSettingsChanged(const FGuitarSettingsData& NewSettings)
{
    ApplyAudioMixFromSettings();
}

void UGuitarAudioMetaSoundBridge::ApplyAudioMixFromSettings()
{
    const UWorld* World = GetWorld();
    const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    const UGuitarGameSettingsSubsystem* SettingsSub = GI ? GI->GetSubsystem<UGuitarGameSettingsSubsystem>() : nullptr;
    if (!SettingsSub)
    {
        return;
    }

    const FGuitarSettingsData& S = SettingsSub->GetSettings();
    const float CombinedVolume = FMath::Clamp(S.MasterVolume * S.GuitarVolume, 0.0f, 1.0f);

    if (AudioComponent)
    {
        AudioComponent->SetVolumeMultiplier(CombinedVolume);
        AudioComponent->SetFloatParameter(TEXT("MasterVolume"), S.MasterVolume);
        AudioComponent->SetFloatParameter(TEXT("GuitarVolume"), S.GuitarVolume);
        AudioComponent->SetFloatParameter(TEXT("ReverbMix"), S.ReverbMix);
    }
}

void UGuitarAudioMetaSoundBridge::HandleGuitarEvent(const FGuitarEvent& Event)
{
    if (GuitarMetaSoundAsset)
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
        return;
    }

    PlayMultiSampleNote(Event);
}

void UGuitarAudioMetaSoundBridge::PlayMultiSampleNote(const FGuitarEvent& Event)
{
    if (VoicePool.IsEmpty())
    {
        return;
    }

    if (SampleZones.IsEmpty())
    {
        LoadSampleZonesFromJson();
    }

    USoundBase* ChosenSound = nullptr;
    float PitchMult = 1.0f;

    if (Event.RightAction == EGuitarRightAction::PercussiveSlap || Event.Articulation == EGuitarArticulation::DeadNote)
    {
        ChosenSound = PercussiveChuckSound;
    }
    else if (const FSampleZone* Zone = FindBestZone(Event.MidiNote, Event.Velocity))
    {
        ChosenSound = Zone->SoundAsset;
        PitchMult = FMath::Pow(2.0f, static_cast<float>(Event.MidiNote - Zone->RootMidi) / 12.0f);
    }

    if (!ChosenSound)
    {
        return;
    }

    const int32 StrSlot = FMath::Clamp(Event.StringIndex - 1, 0, 5);
    if (StringActiveVoiceIdx[StrSlot] >= 0 && VoicePool.IsValidIndex(StringActiveVoiceIdx[StrSlot]))
    {
        if (UAudioComponent* PrevVoice = VoicePool[StringActiveVoiceIdx[StrSlot]])
        {
            if (PrevVoice->IsPlaying())
            {
                PrevVoice->FadeOut(0.03f, 0.0f);
            }
        }
    }

    const int32 VoiceIdx = NextVoiceCursor % VoicePool.Num();
    NextVoiceCursor = (NextVoiceCursor + 1) % VoicePool.Num();
    UAudioComponent* Voice = VoicePool[VoiceIdx];
    if (!Voice)
    {
        return;
    }

    float MasterTimesGuitar = 0.85f;
    if (const UWorld* World = GetWorld())
    {
        if (const UGameInstance* GI = World->GetGameInstance())
        {
            if (const UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
            {
                const FGuitarSettingsData& S = SettingsSub->GetSettings();
                MasterTimesGuitar = FMath::Clamp(S.MasterVolume * S.GuitarVolume, 0.0f, 1.0f);
            }
        }
    }

    float ArticulationGain = 1.0f;
    if (Event.Articulation == EGuitarArticulation::PalmMute) ArticulationGain = 0.65f;

    Voice->SetSound(ChosenSound);
    Voice->SetPitchMultiplier(FMath::Clamp(PitchMult, 0.5f, 2.0f));
    Voice->SetVolumeMultiplier(MasterTimesGuitar * FMath::Clamp(Event.Velocity, 0.12f, 1.0f) * ArticulationGain);
    Voice->Play();
    StringActiveVoiceIdx[StrSlot] = VoiceIdx;
}
