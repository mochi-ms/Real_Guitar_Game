#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GuitarPerformanceTypes.h"
#include "GuitarFretboardComponent.generated.h"

/**
 * Physical 3D fretboard coordinate solver attached at the guitar's nut center.
 * +X runs from nut toward bridge, +Y runs from String 1 (high E) to String 6 (low E), +Z is normal off the fretboard.
 */
UCLASS(ClassGroup = (Guitar), meta = (BlueprintSpawnableComponent))
class REALGUITARGAME_API UGuitarFretboardComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UGuitarFretboardComponent();

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;

    // Standard acoustic guitar scale length in Unreal units (cm)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guitar|Geometry")
    float ScaleLengthCm = 64.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guitar|Geometry")
    float NutSpanCm = 3.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guitar|Geometry")
    float BridgeSpanCm = 5.4f;

    // Distance along +X from nut where the right hand plucks/strums over the soundhole
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guitar|Geometry")
    float SoundholePickXCm = 48.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guitar|IK")
    float FingerInterpSpeed = 18.0f;

    UFUNCTION(BlueprintPure, Category = "Guitar|Geometry")
    FVector GetFretWorldPosition(int32 StringIndex, int32 Fret) const;

    UFUNCTION(BlueprintPure, Category = "Guitar|Geometry")
    FVector GetSoundholeStringWorldPosition(int32 StringIndex) const;

    UFUNCTION(BlueprintPure, Category = "Guitar|IK")
    const FGuitarHandIKTargets& GetSmoothedHandIKTargets() const { return SmoothedIKTargets; }

private:
    FGuitarHandIKTargets SmoothedIKTargets;
};
