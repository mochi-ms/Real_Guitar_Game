#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GuitarPerformanceTypes.h"
#include "GuitarStringVisualComponent.generated.h"

class UMaterialInstanceDynamic;
class UMeshComponent;

/**
 * Tracks 6-string physical vibration energy and pushes shader parameters
 * (StringVibration_1..6, ActiveFret_1..6) onto dynamic guitar string materials.
 */
UCLASS(ClassGroup = (Guitar), meta = (BlueprintSpawnableComponent))
class REALGUITARGAME_API UGuitarStringVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGuitarStringVisualComponent();

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guitar|Visual")
    float VibrationDecayRate = 4.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guitar|Visual")
    float PalmMuteDecayRate = 18.0f;

    UFUNCTION(BlueprintCallable, Category = "Guitar|Visual")
    void RegisterStringMesh(UMeshComponent* TargetMesh, int32 MaterialSlotIndex = 0);

    UFUNCTION(BlueprintPure, Category = "Guitar|Visual")
    float GetStringAmplitude(int32 StringIndex) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void HandleGuitarEvent(const FGuitarEvent& Event);

    TArray<float> StringAmplitudes;
    TArray<float> StringDecayRates;
    TArray<int32> ActiveFrets;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
};
