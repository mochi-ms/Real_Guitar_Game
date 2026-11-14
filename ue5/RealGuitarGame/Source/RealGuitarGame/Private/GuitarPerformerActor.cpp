#include "GuitarPerformerActor.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GuitarAudioMetaSoundBridge.h"
#include "GuitarFretboardComponent.h"
#include "GuitarStringVisualComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AGuitarPerformerActor::AGuitarPerformerActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    GuitarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuitarMesh"));
    GuitarMesh->SetupAttachment(Root);

    // Fretboard nut origin attached to guitar mesh, raised slightly to string top height (+0.35cm)
    Fretboard = CreateDefaultSubobject<UGuitarFretboardComponent>(TEXT("Fretboard"));
    Fretboard->SetupAttachment(GuitarMesh);
    Fretboard->SetRelativeLocation(FVector(0.0f, 0.0f, 0.35f));

    StringVisuals = CreateDefaultSubobject<UGuitarStringVisualComponent>(TEXT("StringVisuals"));
    AudioBridge = CreateDefaultSubobject<UGuitarAudioMetaSoundBridge>(TEXT("AudioBridge"));

    OverviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("OverviewCamera"));
    OverviewCamera->SetupAttachment(Root);
    OverviewCamera->SetRelativeLocation(FVector(42.0f, -95.0f, 38.0f));
    OverviewCamera->SetRelativeRotation(FRotator(-14.0f, 90.0f, 0.0f));
    OverviewCamera->SetFieldOfView(55.0f);
    OverviewCamera->SetActive(true);

    FretboardCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FretboardCamera"));
    FretboardCamera->SetupAttachment(Root);
    FretboardCamera->SetRelativeLocation(FVector(16.0f, -42.0f, 16.0f));
    FretboardCamera->SetRelativeRotation(FRotator(-12.0f, 78.0f, 0.0f));
    FretboardCamera->SetFieldOfView(42.0f);
    FretboardCamera->SetActive(false);

    PickingHandCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PickingHandCamera"));
    PickingHandCamera->SetupAttachment(Root);
    PickingHandCamera->SetRelativeLocation(FVector(52.0f, -48.0f, 20.0f));
    PickingHandCamera->SetRelativeRotation(FRotator(-18.0f, 95.0f, 0.0f));
    PickingHandCamera->SetFieldOfView(42.0f);
    PickingHandCamera->SetActive(false);
}

void AGuitarPerformerActor::BeginPlay()
{
    Super::BeginPlay();

    if (GuitarMesh && !GuitarMesh->GetStaticMesh())
    {
        UStaticMesh* RefGuitar = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Models/AcousticGuitar_Full/StaticMeshes/AcousticGuitar_Full.AcousticGuitar_Full"));
        if (!RefGuitar)
        {
            RefGuitar = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Models/AcousticGuitar_Full.AcousticGuitar_Full"));
        }
        if (!RefGuitar)
        {
            RefGuitar = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Models/AcousticGuitar_Reference.AcousticGuitar_Reference"));
        }
        if (RefGuitar)
        {
            GuitarMesh->SetStaticMesh(RefGuitar);
        }
    }

    if (GuitarMesh && StringVisuals)
    {
        const int32 NumMats = GuitarMesh->GetNumMaterials();
        for (int32 i = 0; i < NumMats; ++i)
        {
            StringVisuals->RegisterStringMesh(GuitarMesh, i);
        }
    }

    if (bShowVirtualHands)
    {
        if (UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
        {
            for (int32 i = 0; i < 4; ++i)
            {
                const FName FingerName(*FString::Printf(TEXT("LeftFinger_%d"), i + 1));
                UStaticMeshComponent* FingerComp = NewObject<UStaticMeshComponent>(this, FingerName);
                if (FingerComp)
                {
                    FingerComp->SetStaticMesh(SphereMesh);
                    FingerComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    FingerComp->SetRelativeScale3D(FVector(0.015f, 0.015f, 0.015f));
                    FingerComp->SetupAttachment(Root);
                    FingerComp->RegisterComponent();
                    if (UMaterialInstanceDynamic* Mat = FingerComp->CreateDynamicMaterialInstance(0))
                    {
                        Mat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.95f, 0.84f, 0.68f, 1.0f));
                    }
                    LeftFingerVisuals.Add(FingerComp);
                }
            }

            RightPickVisual = NewObject<UStaticMeshComponent>(this, TEXT("RightPickVisual"));
            if (RightPickVisual)
            {
                RightPickVisual->SetStaticMesh(SphereMesh);
                RightPickVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                RightPickVisual->SetRelativeScale3D(FVector(0.022f, 0.016f, 0.006f));
                RightPickVisual->SetupAttachment(Root);
                RightPickVisual->RegisterComponent();
                if (UMaterialInstanceDynamic* PickMat = RightPickVisual->CreateDynamicMaterialInstance(0))
                {
                    PickMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.88f, 0.56f, 0.24f, 1.0f));
                }
            }
        }
    }

    SetActiveCameraAngle(CurrentCameraAngle);
}

void AGuitarPerformerActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bShowVirtualHands || !Fretboard)
    {
        return;
    }

    const FGuitarHandIKTargets& Targets = Fretboard->GetSmoothedHandIKTargets();
    for (int32 i = 0; i < LeftFingerVisuals.Num(); ++i)
    {
        UStaticMeshComponent* Comp = LeftFingerVisuals[i];
        if (!Comp || !Targets.LeftFingerWorldTargets.IsValidIndex(i))
        {
            continue;
        }
        Comp->SetWorldLocation(Targets.LeftFingerWorldTargets[i]);
        const bool bActive = Targets.LeftFingerActive.IsValidIndex(i) && Targets.LeftFingerActive[i];
        Comp->SetRelativeScale3D(bActive ? FVector(0.017f) : FVector(0.010f));
    }

    if (RightPickVisual)
    {
        RightPickVisual->SetWorldLocation(Targets.RightHandPickTargetWorld);
    }
}

void AGuitarPerformerActor::SetActiveCameraAngle(EGuitarCameraAngle NewAngle)
{
    CurrentCameraAngle = NewAngle;
    if (OverviewCamera) OverviewCamera->SetActive(NewAngle == EGuitarCameraAngle::Overview);
    if (FretboardCamera) FretboardCamera->SetActive(NewAngle == EGuitarCameraAngle::FretboardCloseUp);
    if (PickingHandCamera) PickingHandCamera->SetActive(NewAngle == EGuitarCameraAngle::PickingHandCloseUp);
}

EGuitarCameraAngle AGuitarPerformerActor::CycleCameraAngle()
{
    const uint8 NextVal = (static_cast<uint8>(CurrentCameraAngle) + 1) % 3;
    SetActiveCameraAngle(static_cast<EGuitarCameraAngle>(NextVal));
    return CurrentCameraAngle;
}

FText AGuitarPerformerActor::GetActiveCameraDisplayName() const
{
    switch (CurrentCameraAngle)
    {
    case EGuitarCameraAngle::FretboardCloseUp:
        return NSLOCTEXT("GuitarHUD", "CamFret", "왼손 프렛 뷰");
    case EGuitarCameraAngle::PickingHandCloseUp:
        return NSLOCTEXT("GuitarHUD", "CamPick", "오른손 주법 뷰");
    default:
        return NSLOCTEXT("GuitarHUD", "CamOverview", "전체 감상 뷰");
    }
}
