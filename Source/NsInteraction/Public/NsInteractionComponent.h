// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NsInteractionTypes.h"
#include "NsInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNsInteractionOptionChangedDelegate, bool, bHasOption, const FNsInteractionOption&, Option);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNsInteractionServerRequestDelegate, const FNsInteractionOption&, Option);

/**
 * Picks the best interaction of an interactor and runs it
 * A sensor gives the targets: the view sensor, an overlap shape, or game code. The component asks each target for its option,
 * keeps the best one, and runs it on a press or at the end of a hold
 * The component does not replicate. A server option fires On Server Request, and the game sends the request to the server
 */
UCLASS(ClassGroup = (NsInteraction), meta = (BlueprintSpawnableComponent))
class NSINTERACTION_API UNsInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsInteractionComponent();

    //~ Begin UActorComponent Interface
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    //~ End UActorComponent Interface

    /** Adds a target and refreshes. A target that is already in the list does nothing */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void AddTarget(class AActor* Actor, class UPrimitiveComponent* Part = nullptr);

    /** Removes every target of this actor and refreshes */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void RemoveTarget(class AActor* Actor);

    /** Replaces the targets with this list and refreshes. The first target wins a tie */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetTargets(const TArray<FNsInteractionTarget>& NewTargets);

    /** Removes every target */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void ClearTargets();

    /** Returns the targets in the order that the sensors gave them */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    const TArray<FNsInteractionTarget>& GetTargets() const { return Targets; }

    /** Adds the overlapped actor. Bind it to On Component Begin Overlap of a sensor shape */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void HandleSensorBeginOverlap(class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    /** Removes the overlapped actor. Bind it to On Component End Overlap of a sensor shape */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void HandleSensorEndOverlap(class UPrimitiveComponent* OverlappedComponent, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    /** Asks each target for its option, keeps the best one, and fires On Option Changed when the shown prompt changes */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void Refresh();

    /** Returns true when a target gives an option now */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool HasOption() const { return bHasOption; }

    /** Returns the best option. Read it only when Has Option is true */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    const FNsInteractionOption& GetOption() const { return Option; }

    /** Starts the input. A press runs at once, a hold waits. Returns true when an option took the input */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool BeginInteract();

    /** Ends the input. A short press on a hold option runs its tap action */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void EndInteract();

    /** Returns the share of the hold in progress, from 0 to 1. 0 without a hold */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    float GetHoldProgress() const;

    /** Server: runs the action of a client request. Check the request first, for example with the view sensor. Returns true when the action ran */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool ExecuteOnServer(class AActor* Actor, class UPrimitiveComponent* Part, FName ActionId);

    /** Asks one target for its option. Returns false when the target is not an interactable or gives no option */
    static bool QueryOption(class AActor* Interactor, const FNsInteractionTarget& Target, FNsInteractionOption& OutOption);

protected:
    /** Returns the score of an option. The highest score wins. Override it for game rules. The default prefers a higher priority, then a nearer target */
    UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
    float ScoreOption(const FNsInteractionOption& Candidate) const;

    virtual float ScoreOption_Implementation(const FNsInteractionOption& Candidate) const;

private:
    /** Runs one action of an option: nothing when the option is disabled, a server request when it needs the server, otherwise the action */
    void RunAction(const FNsInteractionOption& Candidate, FName ActionId);

    /** Owner: asks the server to run an action. Sent when Send Server Requests is true */
    UFUNCTION(Server, Reliable)
    void ServerRequestAction(class AActor* Actor, class UPrimitiveComponent* Part, FName ActionId);

    /** Returns true when the target is in reach of the owner */
    bool IsInReach(const FNsInteractionTarget& Target) const;

    /** Ends the hold in progress without an action */
    void CancelHold();

    /** Ticks only while a hold runs, or while a refresh interval is set */
    void UpdateTickState();

// Variables
public:
    /** Fires when the shown prompt changes: a new target, a new text, or an enable change */
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FNsInteractionOptionChangedDelegate OnOptionChanged;

    /** Fires on a client for a server option. The option names the action. The game sends the target and the action to the server */
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FNsInteractionServerRequestDelegate OnServerRequest;

    /** Seconds between two refreshes while there are targets. 0 refreshes only when the targets change or the input starts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "s"))
    float RefreshInterval;

    /** True sends a server option to the server through this component. False only fires On Server Request, and the game sends it */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bSendServerRequests;

    /** Most distance from the owner to a target. 0 means no limit */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
    float MaxDistanceCm;

private:
    /** Targets from the sensors */
    UPROPERTY(Transient)
    TArray<FNsInteractionTarget> Targets;

    /** Best option of the last refresh */
    UPROPERTY(Transient)
    FNsInteractionOption Option;

    /** True when the last refresh found an option */
    bool bHasOption;

    /** Option of the hold in progress */
    TOptional<FNsInteractionOption> HoldOption;

    /** World time when the hold started */
    double HoldStartSeconds;

    /** Seconds until the next timed refresh */
    float RefreshWait;
};
