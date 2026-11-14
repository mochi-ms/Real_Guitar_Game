#include "GuitarScoreSubsystem.h"
#include "GuitarSongLibrarySubsystem.h"

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

    EnsureLiveBankInitialized();
    ProcessLiveEventsTick(DeltaTime);

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
        SetControlMode(EPlayControlMode::AutoSong);
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
    if (bIsPlaying)
    {
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

    for (const FGuitarEvent& LiveEv : RingingLiveEvents)
    {
        if ((LiveEv.TimeSec + LiveEv.DurationSec) >= LiveClockSec)
        {
            OutActiveEvents.Add(LiveEv);
        }
    }
}

bool UGuitarScoreSubsystem::GetCurrentChord(FGuitarChordSnapshot& OutChord) const
{
    if ((ControlMode == EPlayControlMode::FreePlay || !bIsPlaying) && !CurrentLiveChord.Symbol.IsEmpty())
    {
        OutChord = CurrentLiveChord;
        return true;
    }

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

void UGuitarScoreSubsystem::EnsureLiveBankInitialized()
{
    if (LiveChordSlots.IsEmpty())
    {
        LiveChordSlots = UGuitarSongLibrarySubsystem::GetChordBankSymbols(ChordBankIndex);
    }
    if (CurrentLiveChord.Symbol.IsEmpty() && !LiveChordSlots.IsEmpty())
    {
        ActiveLiveChordSlot = FMath::Clamp(ActiveLiveChordSlot, 0, LiveChordSlots.Num() - 1);
        SelectLiveChordSymbol(LiveChordSlots[ActiveLiveChordSlot]);
    }
}

void UGuitarScoreSubsystem::SetControlMode(EPlayControlMode NewMode)
{
    EnsureLiveBankInitialized();
    if (ControlMode != NewMode)
    {
        ControlMode = NewMode;
        if (ControlMode == EPlayControlMode::FreePlay)
        {
            bIsPlaying = false;
            OnChordChanged.Broadcast(CurrentLiveChord);
        }
        OnControlModeChanged.Broadcast(ControlMode);
    }
}

void UGuitarScoreSubsystem::ToggleControlMode()
{
    if (ControlMode == EPlayControlMode::FreePlay)
    {
        if (bHasScore)
        {
            Play();
        }
    }
    else
    {
        SetControlMode(EPlayControlMode::FreePlay);
    }
}

void UGuitarScoreSubsystem::SetChordBankIndex(int32 BankIndex)
{
    ChordBankIndex = FMath::Clamp(BankIndex, 0, UGuitarSongLibrarySubsystem::GetNumChordBanks() - 1);
    LiveChordSlots = UGuitarSongLibrarySubsystem::GetChordBankSymbols(ChordBankIndex);
    SelectLiveChordSlot(0);
}

TArray<FString> UGuitarScoreSubsystem::GetLiveChordSlots()
{
    EnsureLiveBankInitialized();
    return LiveChordSlots;
}

void UGuitarScoreSubsystem::SelectLiveChordSlot(int32 SlotIndex)
{
    EnsureLiveBankInitialized();
    if (LiveChordSlots.IsValidIndex(SlotIndex))
    {
        ActiveLiveChordSlot = SlotIndex;
        SelectLiveChordSymbol(LiveChordSlots[SlotIndex]);
    }
}

void UGuitarScoreSubsystem::SelectLiveChordSymbol(const FString& Symbol)
{
    TArray<int32> Frets;
    TArray<int32> Fingers;
    UGuitarSongLibrarySubsystem::ResolveChordShape(Symbol, Frets, Fingers);

    CurrentLiveChord.TimeSec = LiveClockSec;
    CurrentLiveChord.DurationSec = 4.0f;
    CurrentLiveChord.Symbol = Symbol;
    CurrentLiveChord.Frets = MoveTemp(Frets);
    CurrentLiveChord.Fingers = MoveTemp(Fingers);

    OnChordChanged.Broadcast(CurrentLiveChord);
}

void UGuitarScoreSubsystem::TriggerLiveStrum(bool bDownStrum, float Velocity)
{
    EnsureLiveBankInitialized();
    if (bIsPlaying || ControlMode != EPlayControlMode::FreePlay)
    {
        SetControlMode(EPlayControlMode::FreePlay);
    }

    const TArray<int32>& Frets = CurrentLiveChord.Frets;
    const TArray<int32>& Fingers = CurrentLiveChord.Fingers;

    TArray<int32> StringsToHit;
    if (bDownStrum)
    {
        for (int32 S = 6; S >= 1; --S)
        {
            if (Frets.IsValidIndex(S - 1) && Frets[S - 1] >= 0)
            {
                StringsToHit.Add(S);
            }
        }
    }
    else
    {
        for (int32 S = 1; S <= 6 && StringsToHit.Num() < 4; ++S)
        {
            if (Frets.IsValidIndex(S - 1) && Frets[S - 1] >= 0)
            {
                StringsToHit.Add(S);
            }
        }
    }

    const float BaseVel = FMath::Clamp(Velocity, 0.2f, 1.0f);
    const int32 GroupId = ++LiveEventCounter;

    for (int32 Idx = 0; Idx < StringsToHit.Num(); ++Idx)
    {
        const int32 S = StringsToHit[Idx];
        const int32 Fret = Frets[S - 1];
        FGuitarEvent Ev;
        Ev.Id = ++LiveEventCounter;
        Ev.TimeSec = LiveClockSec + Idx * (bDownStrum ? 0.0042f : 0.0036f);
        Ev.DurationSec = 1.9f;
        Ev.Beat = 0.0f;
        Ev.StringIndex = S;
        Ev.Fret = Fret;
        Ev.MidiNote = UGuitarSongLibrarySubsystem::ComputeMidiNote(S, Fret);
        Ev.Velocity = FMath::Clamp(BaseVel - Idx * 0.03f, 0.25f, 1.0f);
        Ev.LeftFinger = Fingers.IsValidIndex(S - 1) ? Fingers[S - 1] : 0;
        Ev.RightAction = bDownStrum ? EGuitarRightAction::StrumDown : EGuitarRightAction::StrumUp;
        Ev.Articulation = EGuitarArticulation::LetRing;
        Ev.StrumGroupId = GroupId;
        Ev.ChordSymbol = CurrentLiveChord.Symbol;

        if (Idx == 0)
        {
            EmitLiveNoteNow(Ev);
        }
        else
        {
            PendingLiveEvents.Add(MoveTemp(Ev));
        }
    }
}

void UGuitarScoreSubsystem::PluckLiveString(int32 String1To6, float Velocity)
{
    EnsureLiveBankInitialized();
    if (bIsPlaying || ControlMode != EPlayControlMode::FreePlay)
    {
        SetControlMode(EPlayControlMode::FreePlay);
    }

    const int32 S = FMath::Clamp(String1To6, 1, 6);
    int32 Fret = 0;
    int32 Finger = 0;
    if (CurrentLiveChord.Frets.IsValidIndex(S - 1) && CurrentLiveChord.Frets[S - 1] >= 0)
    {
        Fret = CurrentLiveChord.Frets[S - 1];
        Finger = CurrentLiveChord.Fingers.IsValidIndex(S - 1) ? CurrentLiveChord.Fingers[S - 1] : 0;
    }

    FGuitarEvent Ev;
    Ev.Id = ++LiveEventCounter;
    Ev.TimeSec = LiveClockSec;
    Ev.DurationSec = 1.75f;
    Ev.StringIndex = S;
    Ev.Fret = Fret;
    Ev.MidiNote = UGuitarSongLibrarySubsystem::ComputeMidiNote(S, Fret);
    Ev.Velocity = FMath::Clamp(Velocity, 0.2f, 1.0f);
    Ev.LeftFinger = Finger;
    if (S >= 5)
    {
        Ev.RightAction = EGuitarRightAction::PluckThumb;
    }
    else if (S >= 3)
    {
        Ev.RightAction = EGuitarRightAction::PluckIndex;
    }
    else if (S == 2)
    {
        Ev.RightAction = EGuitarRightAction::PluckMiddle;
    }
    else
    {
        Ev.RightAction = EGuitarRightAction::PluckRing;
    }
    Ev.Articulation = EGuitarArticulation::LetRing;
    Ev.ChordSymbol = CurrentLiveChord.Symbol;

    EmitLiveNoteNow(Ev);
}

void UGuitarScoreSubsystem::TriggerLivePercussive(float Velocity)
{
    EnsureLiveBankInitialized();
    if (bIsPlaying || ControlMode != EPlayControlMode::FreePlay)
    {
        SetControlMode(EPlayControlMode::FreePlay);
    }

    FGuitarEvent Ev;
    Ev.Id = ++LiveEventCounter;
    Ev.TimeSec = LiveClockSec;
    Ev.DurationSec = 0.16f;
    Ev.StringIndex = 4;
    Ev.Fret = 0;
    Ev.MidiNote = 50;
    Ev.Velocity = FMath::Clamp(Velocity, 0.25f, 1.0f);
    Ev.LeftFinger = 0;
    Ev.RightAction = EGuitarRightAction::PercussiveSlap;
    Ev.Articulation = EGuitarArticulation::DeadNote;
    Ev.ChordSymbol = CurrentLiveChord.Symbol;

    EmitLiveNoteNow(Ev);
}

void UGuitarScoreSubsystem::EmitLiveNoteNow(const FGuitarEvent& Ev)
{
    // Keep only the freshest ringing voice per string in RingingLiveEvents
    RingingLiveEvents.RemoveAll([&Ev](const FGuitarEvent& Existing)
    {
        return Existing.StringIndex == Ev.StringIndex;
    });
    RingingLiveEvents.Add(Ev);
    OnGuitarEventTriggered.Broadcast(Ev);
}

void UGuitarScoreSubsystem::ProcessLiveEventsTick(float DeltaTime)
{
    LiveClockSec += DeltaTime;

    for (int32 Idx = 0; Idx < PendingLiveEvents.Num();)
    {
        if (PendingLiveEvents[Idx].TimeSec <= LiveClockSec)
        {
            const FGuitarEvent Ready = PendingLiveEvents[Idx];
            PendingLiveEvents.RemoveAt(Idx);
            EmitLiveNoteNow(Ready);
        }
        else
        {
            ++Idx;
        }
    }

    RingingLiveEvents.RemoveAll([this](const FGuitarEvent& Ev)
    {
        return (Ev.TimeSec + Ev.DurationSec) < LiveClockSec;
    });
}
