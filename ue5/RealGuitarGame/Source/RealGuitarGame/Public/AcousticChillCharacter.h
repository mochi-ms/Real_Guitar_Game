#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AcousticChillCharacter.generated.h"

class AInteractableSeatActor;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;

/**
 * First-person exploration character capable of walking around the photorealistic scene
 * and sitting on any AInteractableSeatActor to watch/listen to auto-played guitar music.
 */
UCLASS()
class REALGUITARGAME_API AAcousticChillCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AAcousticChillCharacter();

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> TogglePlayAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    float InteractTraceDistance = 260.0f;

    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool IsSeated() const { return ActiveSeat != nullptr; }

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void TryToggleSeat();

protected:
    virtual void BeginPlay() override;

private:
    void HandleMove(const FInputActionValue& Value);
    void HandleLook(const FInputActionValue& Value);
    void HandleInteract();
    void HandleTogglePlay();

    AInteractableSeatActor* FindLookedAtSeat() const;

    UPROPERTY(Transient)
    TObjectPtr<AInteractableSeatActor> ActiveSeat;
};
