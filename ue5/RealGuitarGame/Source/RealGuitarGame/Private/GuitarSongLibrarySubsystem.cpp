#include "GuitarSongLibrarySubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GuitarScoreSubsystem.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

void UGuitarSongLibrarySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RefreshSongList();
}

void UGuitarSongLibrarySubsystem::RefreshSongList()
{
    Songs.Reset();

    auto ScanDir = [this](const FString& DirPath, bool bUser)
    {
        if (!FPaths::DirectoryExists(DirPath))
        {
            return;
        }
        TArray<FString> FoundFiles;
        IFileManager::Get().FindFiles(FoundFiles, *FPaths::Combine(DirPath, TEXT("*.json")), true, false);
        FoundFiles.Sort();
        for (const FString& FileName : FoundFiles)
        {
            FGuitarSongLibraryEntry Entry;
            Entry.FilePath = FPaths::Combine(DirPath, FileName);
            Entry.DisplayName = FPaths::GetBaseFilename(FileName).Replace(TEXT(".performance"), TEXT(""));
            Entry.bIsUserScore = bUser;
            Songs.Add(MoveTemp(Entry));
        }
    };

    ScanDir(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Scores")), false);
    ScanDir(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UserScores")), true);

    OnSongLibraryUpdated.Broadcast(Songs);
}

bool UGuitarSongLibrarySubsystem::LoadSongByIndex(int32 Index, bool bAutoPlay)
{
    if (!Songs.IsValidIndex(Index))
    {
        return false;
    }

    UGameInstance* GI = GetGameInstance();
    UWorld* World = GI ? GI->GetWorld() : nullptr;
    UGuitarScoreSubsystem* ScoreSub = World ? World->GetSubsystem<UGuitarScoreSubsystem>() : nullptr;
    if (!ScoreSub)
    {
        return false;
    }

    if (ScoreSub->LoadPerformanceFromJsonFile(Songs[Index].FilePath))
    {
        CurrentIndex = Index;
        if (bAutoPlay)
        {
            ScoreSub->Play();
        }
        return true;
    }
    return false;
}

bool UGuitarSongLibrarySubsystem::PlayNextSong()
{
    if (Songs.IsEmpty())
    {
        return false;
    }
    const int32 NextIdx = (CurrentIndex + 1) % Songs.Num();
    return LoadSongByIndex(NextIdx, true);
}

bool UGuitarSongLibrarySubsystem::PlayPreviousSong()
{
    if (Songs.IsEmpty())
    {
        return false;
    }
    const int32 PrevIdx = (CurrentIndex <= 0) ? (Songs.Num() - 1) : (CurrentIndex - 1);
    return LoadSongByIndex(PrevIdx, true);
}

int32 UGuitarSongLibrarySubsystem::ComputeMidiNote(int32 StringIndex, int32 Fret)
{
    static constexpr int32 StandardTuning[6] = {64, 59, 55, 50, 45, 40};
    const int32 Idx = FMath::Clamp(StringIndex - 1, 0, 5);
    return StandardTuning[Idx] + FMath::Clamp(Fret, 0, 24);
}

bool UGuitarSongLibrarySubsystem::ResolveChordShape(
    const FString& Symbol,
    TArray<int32>& OutFrets,
    TArray<int32>& OutFingers
)
{
    const FString Key = Symbol.TrimStartAndEnd();
    struct FChordDef
    {
        const TCHAR* Name;
        int32 Frets[6];
        int32 Fingers[6];
    };

    // Ordered String 1 (High E) .. String 6 (Low E)
    static const FChordDef Table[] = {
        { TEXT("C"),     {  0, 1, 0, 2, 3, -1 }, { 0, 1, 0, 2, 3, 0 } },
        { TEXT("Cmaj7"), {  0, 0, 0, 2, 3, -1 }, { 0, 0, 0, 2, 3, 0 } },
        { TEXT("Cadd9"), {  0, 3, 0, 2, 3, -1 }, { 0, 3, 0, 1, 2, 0 } },
        { TEXT("G"),     {  3, 0, 0, 0, 2,  3 }, { 4, 0, 0, 0, 1, 2 } },
        { TEXT("G/B"),   {  3, 3, 0, 0, 2, -1 }, { 4, 3, 0, 0, 1, 0 } },
        { TEXT("Am"),    {  0, 1, 2, 2, 0, -1 }, { 0, 1, 3, 2, 0, 0 } },
        { TEXT("Am7"),   {  0, 1, 0, 2, 0, -1 }, { 0, 1, 0, 2, 0, 0 } },
        { TEXT("F"),     {  1, 1, 2, 3, 3,  1 }, { 1, 1, 2, 4, 3, 1 } },
        { TEXT("Fmaj7"), {  0, 1, 2, 3,-1, -1 }, { 0, 1, 2, 3, 0, 0 } },
        { TEXT("D"),     {  2, 3, 2, 0,-1, -1 }, { 2, 3, 1, 0, 0, 0 } },
        { TEXT("Dm"),    {  1, 3, 2, 0,-1, -1 }, { 1, 3, 2, 0, 0, 0 } },
        { TEXT("Dm7"),   {  1, 1, 2, 0,-1, -1 }, { 1, 1, 2, 0, 0, 0 } },
        { TEXT("Em"),    {  0, 0, 0, 2, 2,  0 }, { 0, 0, 0, 3, 2, 0 } },
        { TEXT("Em7"),   {  0, 3, 0, 2, 2,  0 }, { 0, 4, 0, 2, 1, 0 } },
        { TEXT("E"),     {  0, 0, 1, 2, 2,  0 }, { 0, 0, 1, 3, 2, 0 } },
        { TEXT("A"),     {  0, 2, 2, 2, 0, -1 }, { 0, 3, 2, 1, 0, 0 } },
        { TEXT("Bm7"),   {  2, 3, 2, 4, 2, -1 }, { 1, 2, 1, 3, 1, 0 } }
    };

    for (const FChordDef& Def : Table)
    {
        if (Key.Equals(Def.Name, ESearchCase::IgnoreCase))
        {
            OutFrets = TArray<int32>(Def.Frets, 6);
            OutFingers = TArray<int32>(Def.Fingers, 6);
            return true;
        }
    }

    // Default fallback C shape
    OutFrets = { 0, 1, 0, 2, 3, -1 };
    OutFingers = { 0, 1, 0, 2, 3, 0 };
    return false;
}

void UGuitarSongLibrarySubsystem::GenerateAndPlayCustomChart(
    const TArray<FString>& ChordSymbols,
    float Bpm,
    bool bFingerstyle,
    const FString& PresetName,
    bool bAutoPlay
)
{
    TArray<FString> Chords = ChordSymbols;
    if (Chords.IsEmpty())
    {
        Chords = { TEXT("C"), TEXT("G"), TEXT("Am"), TEXT("Fmaj7") };
    }

    FGuitarPerformanceData Data;
    Data.Title = FString::Printf(TEXT("Live Pattern (%s)"), bFingerstyle ? TEXT("Fingerstyle") : TEXT("Strum"));
    Data.Artist = TEXT("In-Game Studio");
    Data.SourceFormat = TEXT("rgchart");
    Data.PlayStyle = bFingerstyle ? TEXT("fingerstyle") : TEXT("strum");
    Data.Bpm = FMath::Clamp(Bpm, 40.0f, 220.0f);
    Data.TuningMidi = { 64, 59, 55, 50, 45, 40 };

    const float SecPerBeat = 60.0f / Data.Bpm;
    constexpr int32 BeatsPerBar = 4;

    TArray<FString> PatternTokens;
    int32 Subdiv = 2;
    if (bFingerstyle)
    {
        if (PresetName == TEXT("travis_pick"))
        {
            PatternTokens = { TEXT("P+a"), TEXT("i"), TEXT("P_alt"), TEXT("m"), TEXT("P"), TEXT("a"), TEXT("P_alt"), TEXT("i") };
        }
        else
        {
            PatternTokens = { TEXT("P"), TEXT("i"), TEXT("m"), TEXT("a"), TEXT("m"), TEXT("i"), TEXT("m"), TEXT("i") };
        }
    }
    else
    {
        if (PresetName == TEXT("16beat_ballad"))
        {
            Subdiv = 4;
            PatternTokens = {
                TEXT("D"), TEXT("-"), TEXT("-"), TEXT("-"),
                TEXT("D"), TEXT("-"), TEXT("D"), TEXT("U"),
                TEXT("-"), TEXT("-"), TEXT("U"), TEXT("-"),
                TEXT("D"), TEXT("-"), TEXT("D"), TEXT("U")
            };
        }
        else if (PresetName == TEXT("pop_rock"))
        {
            PatternTokens = { TEXT("D"), TEXT("-"), TEXT("D"), TEXT("U"), TEXT("X"), TEXT("U"), TEXT("D"), TEXT("U") };
        }
        else
        {
            PatternTokens = { TEXT("D"), TEXT("-"), TEXT("D"), TEXT("U"), TEXT("-"), TEXT("U"), TEXT("D"), TEXT("U") };
        }
    }

    const float StepBeats = 1.0f / static_cast<float>(Subdiv);
    const float StepSec = StepBeats * SecPerBeat;
    const int32 StepsPerBar = BeatsPerBar * Subdiv;

    int32 EventId = 0;
    int32 StrumGroup = 0;

    for (int32 BarIdx = 0; BarIdx < Chords.Num(); ++BarIdx)
    {
        TArray<int32> Frets;
        TArray<int32> Fingers;
        ResolveChordShape(Chords[BarIdx], Frets, Fingers);

        const float BarStartSec = BarIdx * BeatsPerBar * SecPerBeat;
        const float BarDurSec = BeatsPerBar * SecPerBeat;

        FGuitarChordSnapshot Snap;
        Snap.TimeSec = BarStartSec;
        Snap.DurationSec = BarDurSec;
        Snap.Symbol = Chords[BarIdx];
        Snap.Frets = Frets;
        Snap.Fingers = Fingers;
        Data.Chords.Add(MoveTemp(Snap));

        // Find primary & alternating bass strings (String 6..3)
        int32 PrimaryBass = 5;
        int32 AltBass = 4;
        for (int32 S = 6; S >= 3; --S)
        {
            if (Frets[S - 1] >= 0)
            {
                PrimaryBass = S;
                break;
            }
        }
        for (int32 S = PrimaryBass - 1; S >= 3; --S)
        {
            if (Frets[S - 1] >= 0)
            {
                AltBass = S;
                break;
            }
        }

        for (int32 StepIdx = 0; StepIdx < StepsPerBar; ++StepIdx)
        {
            const FString& Tok = PatternTokens[StepIdx % PatternTokens.Num()];
            if (Tok == TEXT("-"))
            {
                continue;
            }

            const float BeatPos = BarIdx * BeatsPerBar + StepIdx * StepBeats;
            const float TimePos = BeatPos * SecPerBeat;
            const float RemainSec = FMath::Max(StepSec, (BarStartSec + BarDurSec) - TimePos);

            if (!bFingerstyle)
            {
                if (Tok == TEXT("X"))
                {
                    ++StrumGroup;
                    for (int32 i = 0; i < 3; ++i)
                    {
                        const int32 S = 4 - i;
                        FGuitarEvent Ev;
                        Ev.Id = EventId++;
                        Ev.TimeSec = TimePos + i * 0.004f;
                        Ev.DurationSec = FMath::Min(0.08f, StepSec * 0.8f);
                        Ev.Beat = BeatPos;
                        Ev.StringIndex = S;
                        Ev.Fret = FMath::Max(0, Frets[S - 1]);
                        Ev.MidiNote = ComputeMidiNote(S, Ev.Fret);
                        Ev.Velocity = 0.55f;
                        Ev.LeftFinger = Fingers[S - 1];
                        Ev.RightAction = EGuitarRightAction::PercussiveSlap;
                        Ev.Articulation = EGuitarArticulation::DeadNote;
                        Ev.StrumGroupId = StrumGroup;
                        Ev.ChordSymbol = Chords[BarIdx];
                        Data.Events.Add(MoveTemp(Ev));
                    }
                }
                else
                {
                    const bool bDown = (Tok == TEXT("D"));
                    TArray<int32> ActiveStrings;
                    if (bDown)
                    {
                        for (int32 S = 6; S >= 1; --S)
                        {
                            if (Frets[S - 1] >= 0) ActiveStrings.Add(S);
                        }
                    }
                    else
                    {
                        for (int32 S = 1; S <= 6 && ActiveStrings.Num() < 4; ++S)
                        {
                            if (Frets[S - 1] >= 0) ActiveStrings.Add(S);
                        }
                    }

                    ++StrumGroup;
                    for (int32 i = 0; i < ActiveStrings.Num(); ++i)
                    {
                        const int32 S = ActiveStrings[i];
                        FGuitarEvent Ev;
                        Ev.Id = EventId++;
                        Ev.TimeSec = TimePos + i * (bDown ? 0.0038f : 0.0032f);
                        Ev.DurationSec = RemainSec;
                        Ev.Beat = BeatPos;
                        Ev.StringIndex = S;
                        Ev.Fret = Frets[S - 1];
                        Ev.MidiNote = ComputeMidiNote(S, Ev.Fret);
                        Ev.Velocity = bDown ? 0.82f : 0.62f;
                        Ev.LeftFinger = Fingers[S - 1];
                        Ev.RightAction = bDown ? EGuitarRightAction::StrumDown : EGuitarRightAction::StrumUp;
                        Ev.Articulation = EGuitarArticulation::LetRing;
                        Ev.StrumGroupId = StrumGroup;
                        Ev.ChordSymbol = Chords[BarIdx];
                        Data.Events.Add(MoveTemp(Ev));
                    }
                }
            }
            else
            {
                TArray<FString> Parts;
                Tok.ParseIntoArray(Parts, TEXT("+"));
                for (const FString& Part : Parts)
                {
                    int32 S = 1;
                    EGuitarRightAction Act = EGuitarRightAction::PluckIndex;
                    if (Part == TEXT("P"))
                    {
                        S = PrimaryBass;
                        Act = EGuitarRightAction::PluckThumb;
                    }
                    else if (Part == TEXT("P_alt"))
                    {
                        S = AltBass;
                        Act = EGuitarRightAction::PluckThumb;
                    }
                    else if (Part == TEXT("i"))
                    {
                        S = (Frets[2] >= 0) ? 3 : 4;
                        Act = EGuitarRightAction::PluckIndex;
                    }
                    else if (Part == TEXT("m"))
                    {
                        S = (Frets[1] >= 0) ? 2 : 3;
                        Act = EGuitarRightAction::PluckMiddle;
                    }
                    else if (Part == TEXT("a"))
                    {
                        S = (Frets[0] >= 0) ? 1 : 2;
                        Act = EGuitarRightAction::PluckRing;
                    }

                    FGuitarEvent Ev;
                    Ev.Id = EventId++;
                    Ev.TimeSec = TimePos;
                    Ev.DurationSec = FMath::Max(StepSec * 1.8f, RemainSec * 0.85f);
                    Ev.Beat = BeatPos;
                    Ev.StringIndex = S;
                    Ev.Fret = FMath::Max(0, Frets[S - 1]);
                    Ev.MidiNote = ComputeMidiNote(S, Ev.Fret);
                    Ev.Velocity = (Act == EGuitarRightAction::PluckThumb) ? 0.85f : 0.72f;
                    Ev.LeftFinger = Fingers[S - 1];
                    Ev.RightAction = Act;
                    Ev.Articulation = EGuitarArticulation::LetRing;
                    Ev.ChordSymbol = Chords[BarIdx];
                    Data.Events.Add(MoveTemp(Ev));
                }
            }
        }
    }

    Data.DurationSec = Chords.Num() * BeatsPerBar * SecPerBeat + 0.5f;

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UWorld* World = GI->GetWorld())
        {
            if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
            {
                ScoreSub->LoadPerformanceData(Data);
                if (bAutoPlay)
                {
                    ScoreSub->Play();
                }
            }
        }
    }
}
