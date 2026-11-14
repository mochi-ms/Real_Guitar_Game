#include "GuitarScoreSubsystem.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

TStatId UGuitarScoreSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGuitarScoreSubsystem, STATGROUP_Tickables);
}

void UGuitarScoreSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bHasScore || !bIsPlaying)
    {
        return;
    }

    CurrentTimeSec += DeltaTime * PlaybackRate;

    const TArray<FGuitarEvent>& Events = LoadedPerformance.Events;
    while (NextEventIndex < Events.Num() && Events[NextEventIndex].TimeSec <= CurrentTimeSec)
    {
        OnGuitarEventTriggered.Broadcast(Events[NextEventIndex]);
        ++NextEventIndex;
    }

    // Update active chord snapshot
    const TArray<FGuitarChordSnapshot>& Chords = LoadedPerformance.Chords;
    int32 NewChordIdx = INDEX_NONE;
    for (int32 i = 0; i < Chords.Num(); ++i)
    {
        const FGuitarChordSnapshot& C = Chords[i];
        if (CurrentTimeSec >= C.TimeSec && CurrentTimeSec < (C.TimeSec + C.DurationSec))
        {
            NewChordIdx = i;
            break;
        }
    }
    if (NewChordIdx != INDEX_NONE && NewChordIdx != CurrentChordIndex)
    {
        CurrentChordIndex = NewChordIdx;
        OnChordChanged.Broadcast(Chords[CurrentChordIndex]);
    }

    if (CurrentTimeSec >= LoadedPerformance.DurationSec)
    {
        if (bLooping)
        {
            SeekTo(0.0f);
        }
        else
        {
            bIsPlaying = false;
        }
    }
}

bool UGuitarScoreSubsystem::LoadPerformanceFromJsonFile(const FString& FilePath)
{
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("GuitarScoreSubsystem: Failed to load score JSON at %s"), *FilePath);
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("GuitarScoreSubsystem: Invalid JSON in %s"), *FilePath);
        return false;
    }

    FGuitarPerformanceData Data;
    Root->TryGetStringField(TEXT("title"), Data.Title);
    Root->TryGetStringField(TEXT("artist"), Data.Artist);
    Root->TryGetStringField(TEXT("source_format"), Data.SourceFormat);
    Root->TryGetStringField(TEXT("play_style"), Data.PlayStyle);
    Root->TryGetNumberField(TEXT("bpm"), Data.Bpm);
    Root->TryGetNumberField(TEXT("capo"), Data.Capo);
    Root->TryGetNumberField(TEXT("duration_sec"), Data.DurationSec);

    const TArray<TSharedPtr<FJsonValue>>* TuningArr = nullptr;
    if (Root->TryGetArrayField(TEXT("tuning_midi"), TuningArr))
    {
        for (const TSharedPtr<FJsonValue>& Val : *TuningArr)
        {
            Data.TuningMidi.Add(static_cast<int32>(Val->AsNumber()));
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* ChordsArr = nullptr;
    if (Root->TryGetArrayField(TEXT("chords"), ChordsArr))
    {
        for (const TSharedPtr<FJsonValue>& Val : *ChordsArr)
        {
            const TSharedPtr<FJsonObject>* Obj = nullptr;
            if (!Val->TryGetObject(Obj))
            {
                continue;
            }
            FGuitarChordSnapshot Snap;
            (*Obj)->TryGetNumberField(TEXT("time_sec"), Snap.TimeSec);
            (*Obj)->TryGetNumberField(TEXT("duration_sec"), Snap.DurationSec);
            (*Obj)->TryGetStringField(TEXT("symbol"), Snap.Symbol);

            const TArray<TSharedPtr<FJsonValue>>* FretsArr = nullptr;
            if ((*Obj)->TryGetArrayField(TEXT("frets"), FretsArr))
            {
                for (const TSharedPtr<FJsonValue>& FVal : *FretsArr)
                {
                    Snap.Frets.Add(static_cast<int32>(FVal->AsNumber()));
                }
            }
            const TArray<TSharedPtr<FJsonValue>>* FingersArr = nullptr;
            if ((*Obj)->TryGetArrayField(TEXT("fingers"), FingersArr))
            {
                for (const TSharedPtr<FJsonValue>& FinVal : *FingersArr)
                {
                    Snap.Fingers.Add(static_cast<int32>(FinVal->AsNumber()));
                }
            }
            Data.Chords.Add(MoveTemp(Snap));
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* EventsArr = nullptr;
    if (Root->TryGetArrayField(TEXT("events"), EventsArr))
    {
        for (const TSharedPtr<FJsonValue>& Val : *EventsArr)
        {
            const TSharedPtr<FJsonObject>* Obj = nullptr;
            if (!Val->TryGetObject(Obj))
            {
                continue;
            }
            FGuitarEvent Ev;
            (*Obj)->TryGetNumberField(TEXT("id"), Ev.Id);
            (*Obj)->TryGetNumberField(TEXT("time_sec"), Ev.TimeSec);
            (*Obj)->TryGetNumberField(TEXT("duration_sec"), Ev.DurationSec);
            (*Obj)->TryGetNumberField(TEXT("beat"), Ev.Beat);
            (*Obj)->TryGetNumberField(TEXT("string"), Ev.StringIndex);
            (*Obj)->TryGetNumberField(TEXT("fret"), Ev.Fret);
            (*Obj)->TryGetNumberField(TEXT("midi_note"), Ev.MidiNote);
            (*Obj)->TryGetNumberField(TEXT("velocity"), Ev.Velocity);
            (*Obj)->TryGetNumberField(TEXT("left_finger"), Ev.LeftFinger);

            FString RightStr;
            if ((*Obj)->TryGetStringField(TEXT("right_action"), RightStr))
            {
                Ev.RightAction = ParseRightAction(RightStr);
            }
            FString ArtStr;
            if ((*Obj)->TryGetStringField(TEXT("articulation"), ArtStr))
            {
                Ev.Articulation = ParseArticulation(ArtStr);
            }
            int32 StrumGroup = -1;
            if ((*Obj)->TryGetNumberField(TEXT("strum_group_id"), StrumGroup))
            {
                Ev.StrumGroupId = StrumGroup;
            }
            (*Obj)->TryGetStringField(TEXT("chord_symbol"), Ev.ChordSymbol);

            Data.Events.Add(MoveTemp(Ev));
        }
    }

    LoadPerformanceData(Data);
    return true;
}

void UGuitarScoreSubsystem::LoadPerformanceData(const FGuitarPerformanceData& InData)
{
    LoadedPerformance = InData;
    bHasScore = true;
    SeekTo(0.0f);
    OnScoreLoaded.Broadcast(LoadedPerformance);
}

void UGuitarScoreSubsystem::Play()
{
    if (bHasScore)
    {
        bIsPlaying = true;
    }
}

void UGuitarScoreSubsystem::Pause()
{
    bIsPlaying = false;
}

void UGuitarScoreSubsystem::Stop()
{
    bIsPlaying = false;
    SeekTo(0.0f);
}

void UGuitarScoreSubsystem::SeekTo(float TimeSec)
{
    CurrentTimeSec = FMath::Max(0.0f, TimeSec);
    CurrentChordIndex = INDEX_NONE;
    NextEventIndex = 0;

    const TArray<FGuitarEvent>& Events = LoadedPerformance.Events;
    while (NextEventIndex < Events.Num() && Events[NextEventIndex].TimeSec < CurrentTimeSec)
    {
        ++NextEventIndex;
    }
}

void UGuitarScoreSubsystem::GetActiveEvents(TArray<FGuitarEvent>& OutActiveEvents) const
{
    OutActiveEvents.Reset();
    for (const FGuitarEvent& Ev : LoadedPerformance.Events)
    {
        if (Ev.TimeSec <= CurrentTimeSec && (Ev.TimeSec + Ev.DurationSec) >= CurrentTimeSec)
        {
            OutActiveEvents.Add(Ev);
        }
        else if (Ev.TimeSec > CurrentTimeSec)
        {
            break;
        }
    }
}

bool UGuitarScoreSubsystem::GetCurrentChord(FGuitarChordSnapshot& OutChord) const
{
    if (LoadedPerformance.Chords.IsValidIndex(CurrentChordIndex))
    {
        OutChord = LoadedPerformance.Chords[CurrentChordIndex];
        return true;
    }
    return false;
}

bool UGuitarScoreSubsystem::GetNextChord(FGuitarChordSnapshot& OutChord) const
{
    const int32 NextIdx = (CurrentChordIndex == INDEX_NONE) ? 0 : (CurrentChordIndex + 1);
    if (LoadedPerformance.Chords.IsValidIndex(NextIdx))
    {
        OutChord = LoadedPerformance.Chords[NextIdx];
        return true;
    }
    return false;
}

EGuitarRightAction UGuitarScoreSubsystem::ParseRightAction(const FString& Value)
{
    if (Value == TEXT("pluck_thumb")) return EGuitarRightAction::PluckThumb;
    if (Value == TEXT("pluck_index")) return EGuitarRightAction::PluckIndex;
    if (Value == TEXT("pluck_middle")) return EGuitarRightAction::PluckMiddle;
    if (Value == TEXT("pluck_ring")) return EGuitarRightAction::PluckRing;
    if (Value == TEXT("pick_down")) return EGuitarRightAction::PickDown;
    if (Value == TEXT("pick_up")) return EGuitarRightAction::PickUp;
    if (Value == TEXT("strum_down")) return EGuitarRightAction::StrumDown;
    if (Value == TEXT("strum_up")) return EGuitarRightAction::StrumUp;
    if (Value == TEXT("percussive_slap")) return EGuitarRightAction::PercussiveSlap;
    return EGuitarRightAction::PluckIndex;
}

EGuitarArticulation UGuitarScoreSubsystem::ParseArticulation(const FString& Value)
{
    if (Value == TEXT("let_ring")) return EGuitarArticulation::LetRing;
    if (Value == TEXT("palm_mute")) return EGuitarArticulation::PalmMute;
    if (Value == TEXT("dead_note")) return EGuitarArticulation::DeadNote;
    if (Value == TEXT("hammer_on")) return EGuitarArticulation::HammerOn;
    if (Value == TEXT("pull_off")) return EGuitarArticulation::PullOff;
    if (Value == TEXT("slide")) return EGuitarArticulation::Slide;
    if (Value == TEXT("harmonic")) return EGuitarArticulation::Harmonic;
    if (Value == TEXT("vibrato")) return EGuitarArticulation::Vibrato;
    return EGuitarArticulation::Normal;
}
