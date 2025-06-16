// Copyright (C) 2024 mykaa. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NsInteractable.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UNsInteractable : public UInterface
{
    GENERATED_BODY()
};

/**
 * Ns Interaction Interface
 */
class NSINTERACTION_API INsInteractable
{
    GENERATED_BODY()

// Functions
public:

    /** On enter the interaction radius */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnEnter(class AActor* InActor);
    virtual void OnEnter_Implementation(class AActor* const InActor) PURE_VIRTUAL(OnEnter, return; );

    /** On exit the interaction radius */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnExit(class AActor* InActor);
    virtual void OnExit_Implementation(class AActor* const InActor) PURE_VIRTUAL(OnExit, return; );

    /** On interact with this object */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnInteract(class AActor* InActor);
    virtual void OnInteract_Implementation(class AActor* const InActor) PURE_VIRTUAL(OnInteract, return; );
};
