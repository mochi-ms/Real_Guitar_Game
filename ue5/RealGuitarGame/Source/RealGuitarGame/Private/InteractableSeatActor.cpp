#include "InteractableSeatActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GuitarPerformerActor.h"
#include "GuitarScoreSubsystem.h"
#include "GuitarSongLibrarySubsystem.h"
#include "Misc/Paths.h"

AInteractableSeatActor::AInteractableSeatActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    SeatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SeatMesh"));
    SeatMesh->SetupAttachment(Root);

    SitAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("SitAnchor"));
    SitAnchor->SetupAttachment(Root);
    SitAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

    PerformanceCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PerformanceCamera"));
    PerformanceCamera->SetupAttachment(Root);
    PerformanceCamera->SetRelativeLocation(FVector(85.0f, -45.0f, 95.0f));
    PerformanceCamera->SetRelativeRotation(FRotator(-12.0f, 155.0f, 0.0f));

    InteractionRadius = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRadius"));
    InteractionRadius->SetupAttachment(Root);
    InteractionRadius->SetSphereRadius(180.0f);
    InteractionRadius->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AInteractableSeatActor::EnterSeat(APlayerController* PC)
{
    if (!PC)
    {
        return;
    }

    AActor* CameraTarget = LinkedPerformer ? static_cast<AActor*>(LinkedPerformer) : static_cast<AActor*>(this);
    PC->SetViewTargetWithBlend(CameraTarget, CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);

    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            if (!DefaultScoreRelativePath.IsEmpty())
            {
                const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), DefaultScoreRelativePath);
                ScoreSub->LoadPerformanceFromJsonFile(FullPath);
                ScoreSub->Play();
            }
            else if (ScoreSub->GetLoadedPerformance().Events.IsEmpty())
            {
                if (UGameInstance* GI = World->GetGameInstance())
                {
                    if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
                    {
                        if (!Lib->LoadSongByIndex(0, true))
                        {
                            // Fallback live balladic progression if no score files exist yet
                            Lib->GenerateAndPlayCustomChart(
                                { TEXT("C"), TEXT("G/B"), TEXT("Am7"), TEXT("Fmaj7") },
                                82.0f,
                                true,
                                TEXT("pima_ballad"),
                                true
                            );
                        }
                    }
                }
            }
            else
            {
                ScoreSub->Play();
            }
        }
    }
}

void AInteractableSeatActor::ExitSeat(APlayerController* PC, AActor* ReturnViewTarget)
{
    if (!PC)
    {
        return;
    }

    if (ReturnViewTarget)
    {
        PC->SetViewTargetWithBlend(ReturnViewTarget, CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
    }

    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->Pause();
        }
    }
}
