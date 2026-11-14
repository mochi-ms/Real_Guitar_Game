#include "GuitarCharacterAnimInstance.h"

#include "Animation/AnimNodeBase.h"
#include "AnimationRuntime.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "TwoBoneIK.h"

namespace
{
    bool ResolveBoneTriple(
        const FBoneContainer& BoneContainer,
        const FName& UpperName,
        const FName& LowerName,
        const FName& EndName,
        FCompactPoseBoneIndex& OutUpper,
        FCompactPoseBoneIndex& OutLower,
        FCompactPoseBoneIndex& OutEnd)
    {
        auto FindCompact = [&BoneContainer](const FName& Name) -> FCompactPoseBoneIndex
        {
            const int32 MeshIdx = BoneContainer.GetPoseBoneIndexForBoneName(Name);
            if (MeshIdx == INDEX_NONE)
            {
                return FCompactPoseBoneIndex(INDEX_NONE);
            }
            return BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIdx));
        };

        OutUpper = FindCompact(UpperName);
        OutLower = FindCompact(LowerName);
        OutEnd = FindCompact(EndName);
        return OutUpper.IsValid() && OutLower.IsValid() && OutEnd.IsValid();
    }

    void SolveTwoBoneLimbCS(
        FCSPose<FCompactPose>& CSPose,
        const FBoneContainer& BoneContainer,
        const FName& UpperName,
        const FName& LowerName,
        const FName& EndName,
        const FVector& JointTargetCS,
        const FVector& EffectorTargetCS,
        float BlendAlpha,
        bool bPreserveEndRotation)
    {
        if (BlendAlpha <= ZERO_ANIMWEIGHT_THRESH)
        {
            return;
        }

        FCompactPoseBoneIndex UpperIdx(INDEX_NONE), LowerIdx(INDEX_NONE), EndIdx(INDEX_NONE);
        if (!ResolveBoneTriple(BoneContainer, UpperName, LowerName, EndName, UpperIdx, LowerIdx, EndIdx))
        {
            return;
        }

        FTransform UpperCS = CSPose.GetComponentSpaceTransform(UpperIdx);
        FTransform LowerCS = CSPose.GetComponentSpaceTransform(LowerIdx);
        FTransform EndCS = CSPose.GetComponentSpaceTransform(EndIdx);
        const FQuat OriginalEndRotCS = EndCS.GetRotation();
        const FTransform OriginalLowerCS = LowerCS;

        AnimationCore::SolveTwoBoneIK(
            UpperCS,
            LowerCS,
            EndCS,
            JointTargetCS,
            EffectorTargetCS,
            false,
            1.0,
            1.0);

        if (bPreserveEndRotation)
        {
            EndCS.SetRotation(OriginalEndRotCS);
        }
        else
        {
            const FQuat ForearmDelta = LowerCS.GetRotation() * OriginalLowerCS.GetRotation().Inverse();
            EndCS.SetRotation((ForearmDelta * OriginalEndRotCS).GetNormalized());
        }

        TArray<FBoneTransform, TFixedAllocator<3>> Transforms;
        Transforms.Emplace(UpperIdx, UpperCS);
        Transforms.Emplace(LowerIdx, LowerCS);
        Transforms.Emplace(EndIdx, EndCS);
        Transforms.Sort(FCompareBoneTransformIndex());
        CSPose.LocalBlendCSBoneTransforms(Transforms, FMath::Clamp(BlendAlpha, 0.0f, 1.0f));
    }

    void ApplyLocalBoneRotationOffset(
        FCompactPose& Pose,
        const FBoneContainer& BoneContainer,
        const FName& BoneName,
        const FRotator& LocalOffsetRot,
        float Weight)
    {
        if (Weight <= ZERO_ANIMWEIGHT_THRESH)
        {
            return;
        }
        const int32 MeshIdx = BoneContainer.GetPoseBoneIndexForBoneName(BoneName);
        if (MeshIdx == INDEX_NONE)
        {
            return;
        }
        const FCompactPoseBoneIndex CompactIdx = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIdx));
        if (CompactIdx.IsValid())
        {
            FTransform& BoneTM = Pose[CompactIdx];
            const FQuat DeltaQ = FQuat::Slerp(FQuat::Identity, LocalOffsetRot.Quaternion(), Weight);
            BoneTM.SetRotation((BoneTM.GetRotation() * DeltaQ).GetNormalized());
        }
    }
}

UGuitarCharacterAnimInstance::UGuitarCharacterAnimInstance()
{
    bUseMultiThreadedAnimationUpdate = false;
}

void UGuitarCharacterAnimInstance::SetGuitarIKTargets(
    const FGuitarHandIKTargets& InTargets,
    bool bInValid,
    float InStrumSwipeOffset)
{
    LatestTargetsWorld = InTargets;
    bHasGuitarTargets = bInValid;
    StrumSwipeOffset = InStrumSwipeOffset;
}

void FGuitarCharacterAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
    FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

    UGuitarCharacterAnimInstance* OwnerInst = Cast<UGuitarCharacterAnimInstance>(InAnimInstance);
    if (!OwnerInst)
    {
        return;
    }

    OwnerInst->AccumTime += DeltaSeconds;
    BreathTime = OwnerInst->AccumTime;
    SitAlpha = OwnerInst->SitAlpha;
    GuitarHoldAlpha = OwnerInst->GuitarHoldAlpha;
    StrumSwipeOffset = OwnerInst->StrumSwipeOffset;
    bHasGuitarTargets = OwnerInst->bHasGuitarTargets;

    if (USkeletalMeshComponent* Driver = OwnerInst->LocomotionDriverMesh.Get())
    {
        CachedDriverLocalPose = Driver->GetBoneSpaceTransforms();
    }
    else
    {
        CachedDriverLocalPose.Reset();
    }

    const USkeletalMeshComponent* TargetMesh = OwnerInst->GetSkelMeshComponent();
    if (!TargetMesh)
    {
        bHasGuitarTargets = false;
        return;
    }

    const FTransform& MeshWorld = TargetMesh->GetComponentTransform();
    const FGuitarHandIKTargets& Targets = OwnerInst->LatestTargetsWorld;

    if (bHasGuitarTargets)
    {
        FVector PalmAnchor = Targets.LeftPalmAnchorWorld;
        FVector FingerSum = FVector::ZeroVector;
        int32 ActiveCount = 0;
        for (int32 F = 0; F < 4; ++F)
        {
            const bool bActive = Targets.LeftFingerActive.IsValidIndex(F) && Targets.LeftFingerActive[F];
            LeftFingersActive[F] = bActive;
            if (bActive && Targets.LeftFingerWorldTargets.IsValidIndex(F))
            {
                FingerSum += Targets.LeftFingerWorldTargets[F];
                ++ActiveCount;
            }
        }
        if (ActiveCount > 0)
        {
            PalmAnchor = FMath::Lerp(PalmAnchor, FingerSum / static_cast<float>(ActiveCount), 0.35f);
        }

        // Convert world-space guitar targets into mesh component space (+X=Left, +Y=Forward, +Z=Up)
        const FVector NeckPosCS = MeshWorld.InverseTransformPosition(PalmAnchor);
        const FVector PickPosCS = MeshWorld.InverseTransformPosition(Targets.RightHandPickTargetWorld);

        // Offset wrists slightly away from string contact point along natural arm entry vectors
        LeftWristTargetCS = NeckPosCS + FVector(2.0f, -2.5f, -5.5f);
        RightWristTargetCS = PickPosCS + FVector(-5.5f, -3.5f, 4.0f + StrumSwipeOffset * 6.5f);
    }
    else
    {
        for (int32 F = 0; F < 4; ++F)
        {
            LeftFingersActive[F] = false;
        }
    }
}

bool FGuitarCharacterAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
    Output.Pose.ResetToRefPose();
    const FBoneContainer& BoneContainer = Output.Pose.GetBoneContainer();

    // 1. Copy live locomotion pose from ABP_Unarmed driver mesh
    if (!CachedDriverLocalPose.IsEmpty())
    {
        for (FCompactPoseBoneIndex CompactIdx : Output.Pose.ForEachBoneIndex())
        {
            const FMeshPoseBoneIndex MeshIdx = BoneContainer.MakeMeshPoseIndex(CompactIdx);
            if (CachedDriverLocalPose.IsValidIndex(MeshIdx.GetInt()))
            {
                Output.Pose[CompactIdx] = CachedDriverLocalPose[MeshIdx.GetInt()];
            }
        }
    }

    // 2. Apply pelvis drop and subtle musician posture before computing Component Space poses
    if (SitAlpha > ZERO_ANIMWEIGHT_THRESH)
    {
        const int32 PelvisMeshIdx = BoneContainer.GetPoseBoneIndexForBoneName(TEXT("pelvis"));
        if (PelvisMeshIdx != INDEX_NONE)
        {
            const FCompactPoseBoneIndex PelvisIdx = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(PelvisMeshIdx));
            if (PelvisIdx.IsValid())
            {
                Output.Pose[PelvisIdx].AddToTranslation(FVector(0.0f, -2.0f * SitAlpha, -38.0f * SitAlpha));
            }
        }
    }

    const float SwayAngle = FMath::Sin(BreathTime * 1.45f) * 2.2f * GuitarHoldAlpha;
    if (GuitarHoldAlpha > ZERO_ANIMWEIGHT_THRESH)
    {
        ApplyLocalBoneRotationOffset(Output.Pose, BoneContainer, TEXT("spine_02"), FRotator(4.0f + SwayAngle * 0.4f, 0.0f, SwayAngle), GuitarHoldAlpha);
        ApplyLocalBoneRotationOffset(Output.Pose, BoneContainer, TEXT("head"), FRotator(6.0f, 8.0f, -SwayAngle * 0.5f), GuitarHoldAlpha);
    }

    if (SitAlpha <= ZERO_ANIMWEIGHT_THRESH && GuitarHoldAlpha <= ZERO_ANIMWEIGHT_THRESH)
    {
        return true;
    }

    FCSPose<FCompactPose> CSPose;
    CSPose.InitPose(Output.Pose);

    // 3. Seated Two-Bone Leg IK (thigh -> calf -> foot resting flat on deck/rug)
    if (SitAlpha > ZERO_ANIMWEIGHT_THRESH)
    {
        SolveTwoBoneLimbCS(
            CSPose, BoneContainer,
            TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"),
            FVector(16.0f, 70.0f, 52.0f),
            FVector(14.5f, 36.0f, 8.4f),
            SitAlpha,
            true);

        SolveTwoBoneLimbCS(
            CSPose, BoneContainer,
            TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"),
            FVector(-16.0f, 70.0f, 52.0f),
            FVector(-14.5f, 36.0f, 8.4f),
            SitAlpha,
            true);
    }

    // 4. Dual-Arm Two-Bone Guitar Performance IK
    if (GuitarHoldAlpha > ZERO_ANIMWEIGHT_THRESH)
    {
        const float UpperArmZDrop = -38.0f * SitAlpha;
        const FVector FallbackLeftWrist = FVector(28.0f, 29.0f, 110.0f + UpperArmZDrop);
        const FVector FallbackRightWrist = FVector(-14.0f, 28.0f, 102.0f + UpperArmZDrop);

        const FVector LeftTarget = bHasGuitarTargets ? LeftWristTargetCS : FallbackLeftWrist;
        const FVector RightTarget = bHasGuitarTargets ? RightWristTargetCS : FallbackRightWrist;

        const FVector LeftElbow = FVector(38.0f, -4.0f, 92.0f + UpperArmZDrop);
        const FVector RightElbow = FVector(-44.0f, -8.0f, 114.0f + UpperArmZDrop);

        SolveTwoBoneLimbCS(
            CSPose, BoneContainer,
            TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l"),
            LeftElbow,
            LeftTarget,
            GuitarHoldAlpha,
            false);

        SolveTwoBoneLimbCS(
            CSPose, BoneContainer,
            TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r"),
            RightElbow,
            RightTarget,
            GuitarHoldAlpha,
            false);
    }

    FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(CSPose, Output.Pose);

    // 5. Left-hand finger fretting curl + Right-hand strum/pick articulation in local bone space
    if (GuitarHoldAlpha > ZERO_ANIMWEIGHT_THRESH)
    {
        static const FName FingerNames01[4] = { TEXT("index_01_l"), TEXT("middle_01_l"), TEXT("ring_01_l"), TEXT("pinky_01_l") };
        static const FName FingerNames02[4] = { TEXT("index_02_l"), TEXT("middle_02_l"), TEXT("ring_02_l"), TEXT("pinky_02_l") };
        for (int32 F = 0; F < 4; ++F)
        {
            const float Curl = LeftFingersActive[F] ? 1.0f : 0.42f;
            ApplyLocalBoneRotationOffset(Output.Pose, BoneContainer, FingerNames01[F], FRotator(0.0f, 0.0f, -42.0f * Curl), GuitarHoldAlpha);
            ApplyLocalBoneRotationOffset(Output.Pose, BoneContainer, FingerNames02[F], FRotator(0.0f, 0.0f, -48.0f * Curl), GuitarHoldAlpha);
        }
        // Right picking hand slight cup over soundboard
        static const FName RightFingerNames[4] = { TEXT("index_01_r"), TEXT("middle_01_r"), TEXT("ring_01_r"), TEXT("pinky_01_r") };
        const float PickCurl = 0.65f + FMath::Abs(StrumSwipeOffset) * 0.25f;
        for (int32 F = 0; F < 4; ++F)
        {
            ApplyLocalBoneRotationOffset(Output.Pose, BoneContainer, RightFingerNames[F], FRotator(0.0f, 0.0f, 34.0f * PickCurl), GuitarHoldAlpha);
        }
    }

    return true;
}
