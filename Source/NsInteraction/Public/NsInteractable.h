// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NsInteractionTypes.h"
#include "UObject/Interface.h"
#include "NsInteractable.generated.h"

UINTERFACE(BlueprintType, MinimalAPI)
class UNsInteractable : public UInterface
{
    GENERATED_BODY()
};

/**
 * An actor that offers interactions. Implement it in C++ or in Blueprint
 * The interaction component asks for an option, shows it, and runs it when the input comes
 */
class NSINTERACTION_API INsInteractable
{
    GENERATED_BODY()

// Functions
public:
    /** Fills the option for this interactor and part. Return false when the part has no action now */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool GetInteractionOption(class AActor* Interactor, class UPrimitiveComponent* Part, FNsInteractionOption& OutOption);

    virtual bool GetInteractionOption_Implementation(class AActor* Interactor, class UPrimitiveComponent* Part, FNsInteractionOption& OutOption);

    /** Runs the action that Option.ActionId names. Return false when the action did not run */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool ExecuteInteraction(class AActor* Interactor, const FNsInteractionOption& Option);

    virtual bool ExecuteInteraction_Implementation(class AActor* Interactor, const FNsInteractionOption& Option);

    /** An interactor starts to consider this actor, for example when it comes near or looks at it */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnInteractorEnter(class AActor* Interactor);

    virtual void OnInteractorEnter_Implementation(class AActor* Interactor);

    /** An interactor stops to consider this actor */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnInteractorExit(class AActor* Interactor);

    virtual void OnInteractorExit_Implementation(class AActor* Interactor);
};
