#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableSeatActor.generated.h"

class UCameraComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Bench or chair in the photorealistic environment that transitions the player into
 * guitar listening/performance view when interacted with.
 */
UCLASS()
class REALGUITARGAME_API AInteractableSeatActor : public AActor
{
    GENERATED_BODY()

public:
    AInteractableSeatActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat")
    TObjectPtr<UStaticMeshComponent> SeatMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat")
    TObjectPtr<USceneComponent> SitAnchor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat")
    TObjectPtr<UCameraComponent> PerformanceCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat")
    TObjectPtr<USphereComponent> InteractionRadius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seat|Performance")
    float CameraBlendTime = 0.75f;

    // Optional relative path inside ProjectContentDir, e.g. "Scores/fingerstyle_canon.performance.json"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seat|Performance")
    FString DefaultScoreRelativePath;

    UFUNCTION(BlueprintCallable, Category = "Seat")
    void EnterSeat(APlayerController* PC);

    UFUNCTION(BlueprintCallable, Category = "Seat")
    void ExitSeat(APlayerController* PC, AActor* ReturnViewTarget);
};
