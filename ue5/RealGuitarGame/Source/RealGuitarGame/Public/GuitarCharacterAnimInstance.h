#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "GuitarPerformanceTypes.h"
#include "GuitarCharacterAnimInstance.generated.h"

class USkeletalMeshComponent;

USTRUCT()
struct FGuitarCharacterAnimInstanceProxy : public FAnimInstanceProxy
{
    GENERATED_BODY()

    FGuitarCharacterAnimInstanceProxy() = default;
    explicit FGuitarCharacterAnimInstanceProxy(UAnimInstance* Instance)
        : FAnimInstanceProxy(Instance)
    {
    }

    virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
    virtual bool Evaluate(FPoseContext& Output) override;

    TArray<FTransform> CachedDriverLocalPose;
    float SitAlpha = 0.0f;
    float GuitarHoldAlpha = 0.0f;
    bool bHasGuitarTargets = false;
    FVector LeftWristTargetCS = FVector::ZeroVector;
    FVector RightWristTargetCS = FVector::ZeroVector;
    FVector LeftElbowHintCS = FVector(34.0f, 4.0f, 96.0f);
    FVector RightElbowHintCS = FVector(-40.0f, -6.0f, 108.0f);
    TStaticArray<bool, 4> LeftFingersActive{ InPlace, false };
    float StrumSwipeOffset = 0.0f;
    float BreathTime = 0.0f;
};

/**
 * Procedural full-body & Two-Bone IK Animation Instance driving Quinn/Manny:
 * - Copies live locomotion (Idle / Walk / Jog / Jump) from LocomotionDriverMesh (ABP_Unarmed)
 * - Blends seated hip/knee/ankle leg posture onto bench or stool
 * - Solves component-space Two-Bone IK for both arms and curls fretting fingers onto the live guitar fretboard
 */
UCLASS()
class REALGUITARGAME_API UGuitarCharacterAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UGuitarCharacterAnimInstance();

    void SetLocomotionDriverMesh(USkeletalMeshComponent* InDriverMesh) { LocomotionDriverMesh = InDriverMesh; }
    void SetSitAlpha(float InAlpha) { SitAlpha = FMath::Clamp(InAlpha, 0.0f, 1.0f); }
    void SetGuitarHoldAlpha(float InAlpha) { GuitarHoldAlpha = FMath::Clamp(InAlpha, 0.0f, 1.0f); }
    void SetGuitarIKTargets(const FGuitarHandIKTargets& InTargets, bool bInValid, float InStrumSwipeOffset);

protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return &Proxy; }
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override {}

private:
    friend struct FGuitarCharacterAnimInstanceProxy;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> LocomotionDriverMesh;

    FGuitarCharacterAnimInstanceProxy Proxy;
    FGuitarHandIKTargets LatestTargetsWorld;
    float SitAlpha = 0.0f;
    float GuitarHoldAlpha = 0.0f;
    float StrumSwipeOffset = 0.0f;
    bool bHasGuitarTargets = false;
    float AccumTime = 0.0f;
};
