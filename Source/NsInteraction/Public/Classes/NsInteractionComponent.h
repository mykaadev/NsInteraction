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
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    /** Sets the component that the view trace starts from and points along, such as a first-person camera */
    UFUNCTION(BlueprintCallable, Category = "Interaction|View Trace")
    void SetViewSource(USceneComponent* InViewSource);

    /** Traces from the view source, queues the aimed actor, and resolves. The tick calls it while bUseViewTrace is on */
    UFUNCTION(BlueprintCallable, Category = "Interaction|View Trace")
    void RefreshFromView();

    /** Resolves the queue and fires OnInteractionChanged when the shown prompt changes. Call it after the queue changes */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void RefreshInteraction();

    /** Returns the last resolved interaction */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    const FNsResolvedInteraction& GetCurrentInteraction() const { return CurrentInteraction; }

    /** Returns the part that the view trace hits, or null */
    UFUNCTION(BlueprintPure, Category = "Interaction|View Trace")
    UPrimitiveComponent* GetFocusComponent() const;

    /** Returns the aimed part of an interactor. An interactable calls this while it builds its candidate */
    UFUNCTION(BlueprintPure, Category = "Interaction|View Trace")
    static UPrimitiveComponent* GetFocusComponentOf(const class AActor* Interactor);

    /** Starts the input on the current interaction. A press runs at once, a hold waits. Returns true when an interaction took the input */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool BeginInteract();

    /** Ends the input. A short press on a hold candidate runs its tap action */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void EndInteract();

    /** Returns the share of the current hold, from 0 to 1. 0 without a hold */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    float GetHoldProgress() const;

    /** Runs a request from OnAuthorityRequest on the authority. The game checks the request first */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool InteractAsAuthority(class AActor* TargetActor, UPrimitiveComponent* TargetComponent);

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

    /** Fires when the shown prompt changes: a new target, a new part, a new text, or an enable change */
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FNsInteractionChangedDelegate OnInteractionChanged;

    /** Fires on a client for an authority candidate. The game sends TargetActor and TargetComponent to the authority */
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FNsInteractionAuthorityRequestDelegate OnAuthorityRequest;

// Variables
public:

    /** Holds all the Current Interactable Actors */
    UPROPERTY(BlueprintReadWrite)
    TArray<class AActor*> InteractionQueue;

    /** Shared distance used for prompt resolution and interaction execution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
    float InteractionRadiusCm;

    /** Traces from the view source every frame on the machine that controls the owner */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|View Trace")
    bool bUseViewTrace;

    /** Length of the view trace */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|View Trace", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bUseViewTrace"))
    float ViewTraceDistanceCm;

    /** Channel of the view trace */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|View Trace", meta = (EditCondition = "bUseViewTrace"))
    TEnumAsByte<ECollisionChannel> ViewTraceChannel;

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

    /** Runs one candidate: nothing when disabled, a request when it needs the authority, otherwise its action */
    void RunCandidate(const FNsInteractionCandidate& Candidate);

    /** Start of the view trace */
    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> ViewSource;

    /** Last hit of the view trace */
    FHitResult FocusHit;

    /** Last resolved interaction */
    FNsResolvedInteraction CurrentInteraction;

    /** Candidate of the hold in progress */
    TOptional<FNsInteractionCandidate> HoldCandidate;

    /** World time when the hold started */
    double HoldStartSeconds = 0.0;

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
