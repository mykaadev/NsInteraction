// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NsInteractionTypeLibrary.h"
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
    virtual void OnEnter_Implementation(class AActor* const InActor);

    /** On exit the interaction radius */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnExit(class AActor* InActor);
    virtual void OnExit_Implementation(class AActor* const InActor);

    /** On interact with this object. Used as the fallback when a resolved candidate does not execute explicitly. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnInteract(class AActor* InActor);
    virtual void OnInteract_Implementation(class AActor* const InActor);

    /** Returns the world location used by simple focus systems. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    FVector GetInteractionWorldLocation() const;
    virtual FVector GetInteractionWorldLocation_Implementation() const;

    /** Builds the current interaction candidate for this interactable. Return false when there is nothing usable. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool GetInteractionCandidate(class AActor* InActor, UPARAM(ref) FNsInteractionCandidate& OutCandidate);
    virtual bool GetInteractionCandidate_Implementation(class AActor* const InActor, FNsInteractionCandidate& OutCandidate);

    /** Executes a resolved interaction candidate. Return false to fall back to OnInteract. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool ExecuteInteractionCandidate(class AActor* InActor, const FNsInteractionCandidate& Candidate);
    virtual bool ExecuteInteractionCandidate_Implementation(class AActor* const InActor, const FNsInteractionCandidate& Candidate);
};
