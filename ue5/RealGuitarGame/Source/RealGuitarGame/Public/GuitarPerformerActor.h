#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GuitarPerformerActor.generated.h"

class UCameraComponent;
class UGuitarAudioMetaSoundBridge;
class UGuitarFretboardComponent;
class UGuitarStringVisualComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EGuitarCameraAngle : uint8
{
    Overview UMETA(DisplayName = "전체 감상 뷰"),
    FretboardCloseUp UMETA(DisplayName = "왼손 프렛 클로즈업"),
    PickingHandCloseUp UMETA(DisplayName = "오른손 피킹/스트로크 클로즈업")
};

/**
 * Complete acoustic/electric guitar stage actor combining 3D mesh, fretboard IK solver,
 * per-string vibration shader driver, MetaSound audio bridge, and multi-angle cameras.
 */
UCLASS()
class REALGUITARGAME_API AGuitarPerformerActor : public AActor
{
    GENERATED_BODY()

public:
    AGuitarPerformerActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guitar")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guitar")
    TObjectPtr<UStaticMeshComponent> GuitarMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guitar")
    TObjectPtr<UGuitarFretboardComponent> Fretboard;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guitar")
    TObjectPtr<UGuitarStringVisualComponent> StringVisuals;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guitar")
    TObjectPtr<UGuitarAudioMetaSoundBridge> AudioBridge;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guitar|Camera")
    TObjectPtr<UCameraComponent> OverviewCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guitar|Camera")
    TObjectPtr<UCameraComponent> FretboardCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guitar|Camera")
    TObjectPtr<UCameraComponent> PickingHandCamera;

    UFUNCTION(BlueprintCallable, Category = "Guitar|Camera")
    void SetActiveCameraAngle(EGuitarCameraAngle NewAngle);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Camera")
    EGuitarCameraAngle CycleCameraAngle();

    UFUNCTION(BlueprintPure, Category = "Guitar|Camera")
    EGuitarCameraAngle GetActiveCameraAngle() const { return CurrentCameraAngle; }

    UFUNCTION(BlueprintPure, Category = "Guitar|Camera")
    FText GetActiveCameraDisplayName() const;

protected:
    virtual void BeginPlay() override;

private:
    EGuitarCameraAngle CurrentCameraAngle = EGuitarCameraAngle::Overview;
};
