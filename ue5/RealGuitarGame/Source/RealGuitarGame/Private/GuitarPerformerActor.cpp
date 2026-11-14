#include "GuitarPerformerActor.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GuitarAudioMetaSoundBridge.h"
#include "GuitarFretboardComponent.h"
#include "GuitarStringVisualComponent.h"

AGuitarPerformerActor::AGuitarPerformerActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    GuitarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuitarMesh"));
    GuitarMesh->SetupAttachment(Root);

    // Fretboard nut origin attached to guitar mesh
    Fretboard = CreateDefaultSubobject<UGuitarFretboardComponent>(TEXT("Fretboard"));
    Fretboard->SetupAttachment(GuitarMesh);

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

    if (GuitarMesh && StringVisuals)
    {
        const int32 NumMats = GuitarMesh->GetNumMaterials();
        for (int32 i = 0; i < NumMats; ++i)
        {
            StringVisuals->RegisterStringMesh(GuitarMesh, i);
        }
    }
    SetActiveCameraAngle(CurrentCameraAngle);
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
