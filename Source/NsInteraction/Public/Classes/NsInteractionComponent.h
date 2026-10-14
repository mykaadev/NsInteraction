// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NsInteractionTypeLibrary.h"
#include "NsInteractionComponent.generated.h"

class INsInteractable;

/**
 * Interaction Component
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NSINTERACTION_API UNsInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

// Functions
public:

    /** Constructor */
    UNsInteractionComponent();

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    //~ End UActorComponent Interface

    /** Adds an item to the interaction queue */
    UFUNCTION(BlueprintCallable)
    void AddToInteractionQueue(class AActor* const InActor);

    /** Removes an item from the interaction queue */
    UFUNCTION(BlueprintCallable)
    void RemoveFromInteractionQueue(class AActor* const InActor);

    /** Removes every item from the interaction queue */
    UFUNCTION(BlueprintCallable)
    void ClearInteractionQueue();

    /** Synchronizes the interaction queue to a newly observed set of nearby actors. */
    UFUNCTION(BlueprintCallable)
    void SyncInteractionQueue(const TArray<class AActor*>& NearbyActors);

    /** Adds an overlapped interactable actor to the queue. */
    UFUNCTION(BlueprintCallable)
    void HandleInteractionSensorBeginOverlap(class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    /** Removes an overlapped interactable actor from the queue. */
    UFUNCTION(BlueprintCallable)
    void HandleInteractionSensorEndOverlap(class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    /** Resolve the best candidate from the current queue. */
    UFUNCTION(BlueprintCallable)
    FNsResolvedInteraction ResolveCurrentInteraction() const;

    /** Resolve the best candidate from an explicit candidate list. */
    UFUNCTION(BlueprintCallable)
    FNsResolvedInteraction ResolveInteractionCandidates(const TArray<FNsInteractionCandidate>& Candidates) const;

    /** Returns whether the actor is inside this component's interaction distance. */
    UFUNCTION(BlueprintCallable)
    bool IsActorInInteractionRange(class AActor* InActor) const;

    /** Interact with the current object */
    UFUNCTION(BlueprintCallable)
    void Interact();

    /** Interact with the currently resolved candidate. */
    UFUNCTION(BlueprintCallable)
    void InteractResolved();

    /** Interact with an already resolved candidate. */
    UFUNCTION(BlueprintCallable)
    void InteractWithResolvedInteraction(const FNsResolvedInteraction& ResolvedInteraction);

// Variables
public:

    /** Holds all the Current Interactable Actors */
    UPROPERTY(BlueprintReadWrite)
    TArray<class AActor*> InteractionQueue;

    /** Shared distance used for prompt resolution and interaction execution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
    float InteractionRadiusCm;

    /** Enables log output for candidate count and selected interaction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bLogInteractionResolution;

    /** Candidates this close are chosen by distance/facing only, ignoring priority bias. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
    float CloseIntentDistanceCm;

    /** Score distance gained or lost per priority step outside the close intent distance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
    float PriorityScoreStepCm;

    /** Maximum facing penalty in centimeters when the interactor faces away from the candidate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
    float FacingScoreWeightCm;

private:

    /** Resolves the actor that owns this component, including typed outer fallback for tests. */
    class AActor* ResolveInteractionOwner() const;

    /** Calls OnEnter on native and Blueprint interactables. */
    static void NotifyInteractableEnter(class AActor* InteractableActor, class AActor* OwnerActor);

    /** Calls OnExit on native and Blueprint interactables. */
    static void NotifyInteractableExit(class AActor* InteractableActor, class AActor* OwnerActor);

    /** Calls OnInteract on native and Blueprint interactables. */
    static void NotifyInteractableUsed(class AActor* InteractableActor, class AActor* OwnerActor);

    /** Requests the current interaction candidate from an interactable actor. */
    static bool GetInteractionCandidateFromActor(class AActor* InteractableActor, class AActor* OwnerActor, FNsInteractionCandidate& OutCandidate);

    /** Executes a resolved candidate on an interactable actor. */
    static bool ExecuteInteractionCandidateOnActor(class AActor* InteractableActor, class AActor* OwnerActor, const FNsInteractionCandidate& Candidate);

    /** Returns the point used for distance checks. */
    static FVector ResolveActorInteractionLocation(class AActor* InActor);

    /** Returns the point used to score an interaction candidate. */
    static FVector ResolveCandidateInteractionLocation(const FNsInteractionCandidate& Candidate);

    /** Calculates candidate facing penalty from the owner actor, when available. */
    static float CalculateFacingScore(class AActor* OwnerActor, const FNsInteractionCandidate& Candidate, float FacingWeightCm);

    /** Calculates the final resolver score for a candidate. */
    float CalculateFinalScore(float DistanceCm, float FacingScoreCm, ENsInteractionPriority Priority) const;

    /** Converts priority enum to a small integer bias. */
    static int32 GetPriorityValue(ENsInteractionPriority Priority);
};
