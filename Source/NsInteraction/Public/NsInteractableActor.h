// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NsInteractable.h"
#include "NsInteractionTypes.h"
#include "NsInteractableActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNsInteractedDelegate, class AActor*, Interactor, FName, ActionId);

/**
 * Actor with one interaction option that the Blueprint sets in the details panel
 * A child Blueprint adds its meshes and handles On Interacted. The default option runs on the server, so every player sees the result
 * Override Get Interaction Option in the Blueprint for options that change with the game state
 */
UCLASS(Blueprintable)
class NSINTERACTION_API ANsInteractableActor : public AActor, public INsInteractable
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    ANsInteractableActor();

    //~ Begin INsInteractable Interface
    virtual bool GetInteractionOption_Implementation(AActor* Interactor, UPrimitiveComponent* Part, FNsInteractionOption& OutOption) override;
    virtual bool ExecuteInteraction_Implementation(AActor* Interactor, const FNsInteractionOption& InOption) override;
    //~ End INsInteractable Interface

    /** Turns the option on or off. A disabled option shows its prompt but runs nothing */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetInteractionEnabled(bool bEnabled);

protected:
    /** Runs the action on the machine of the option: the server for a server option. Implement the result here */
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction", DisplayName = "On Interact")
    void ReceiveInteract(AActor* Interactor, FName ActionId);

// Variables
public:
    /** Option that this actor offers. The interaction component fills the target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FNsInteractionOption Option;

    /** Fires after On Interact, on the machine that ran the action */
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FNsInteractedDelegate OnInteracted;
};
