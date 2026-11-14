#include "InteractableSeatActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GuitarScoreSubsystem.h"
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

    PC->SetViewTargetWithBlend(this, CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);

    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            if (!DefaultScoreRelativePath.IsEmpty())
            {
                const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), DefaultScoreRelativePath);
                ScoreSub->LoadPerformanceFromJsonFile(FullPath);
            }
            ScoreSub->Play();
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
