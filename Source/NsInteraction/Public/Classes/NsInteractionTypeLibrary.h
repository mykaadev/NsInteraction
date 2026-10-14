// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NsInteractionTypeLibrary.generated.h"

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FNsInteractionExecuteDynamicDelegate, AActor*, Interactor);

/**
 * Generic priority bias used only when distance/facing do not make player intent clear.
 */
UENUM(BlueprintType)
enum class ENsInteractionPriority : uint8
{
    Low UMETA(DisplayName = "Low"),
    Normal UMETA(DisplayName = "Normal"),
    High UMETA(DisplayName = "High"),
    Critical UMETA(DisplayName = "Critical")
};

/**
 * Executable action shared by Blueprint and native interaction candidates.
 */
USTRUCT(BlueprintType)
struct NSINTERACTION_API FNsInteractionAction
{
    GENERATED_BODY()

    /** Blueprint execution hook used when this action runs */
    UPROPERTY(BlueprintReadWrite, Category = "Interaction")
    FNsInteractionExecuteDynamicDelegate Dynamic;

    /** Native execution hook used when this action runs */
    TFunction<bool(AActor*)> Native;

    /** Clears both Blueprint and native bindings */
    void Reset();

    /** Assigns a native action */
    void BindNative(TFunction<bool(AActor*)> InNative);

    /** Assigns a Blueprint action */
    void BindBlueprint(const FNsInteractionExecuteDynamicDelegate& InDynamic);

    /** Returns true when this action has any binding */
    bool IsBound() const;

    /** Runs the bound action, when present */
    bool Execute(AActor* Interactor) const;
};

/**
 * Describes one executable interaction option offered by an interactable actor.
 */
USTRUCT(BlueprintType)
struct NSINTERACTION_API FNsInteractionCandidate
{
    GENERATED_BODY()

    /** Actor that owns and executes this interaction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TObjectPtr<AActor> TargetActor;

    /** Text shown to the player when this candidate wins. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText PromptText;

    /** World location used by UI systems to position the prompt. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FVector PromptWorldLocation;

    /** Priority bias used after distance/facing have been scored. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    ENsInteractionPriority Priority;

    /** Action run when this candidate wins */
    UPROPERTY(BlueprintReadWrite, Category = "Interaction")
    FNsInteractionAction Action;

    FNsInteractionCandidate();

    /** Returns true when this candidate has an action bound */
    bool HasExecute() const;

    /** Runs the bound action, when present. Returns false when unbound or rejected */
    bool TryExecute(AActor* Interactor) const;

};

/**
 * Result of resolving an interaction queue or explicit candidate list.
 */
USTRUCT(BlueprintType)
struct NSINTERACTION_API FNsResolvedInteraction
{
    GENERATED_BODY()

    /** True when a usable candidate was selected. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bHasInteraction;

    /** Winning candidate. Only meaningful when bHasInteraction is true. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FNsInteractionCandidate Candidate;

    /** Valid candidates sorted from best to worst. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TArray<FNsInteractionCandidate> Candidates;

    FNsResolvedInteraction();
};

/**
 * Blueprint helpers for interaction candidates
 */
UCLASS()
class NSINTERACTION_API UNsInteractionCandidateLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

// Functions
public:

    /** Assigns the event that runs when this candidate wins interaction resolution */
    UFUNCTION(BlueprintCallable, Category = "Interaction", meta = (DisplayName = "Set Interaction Candidate Action"))
    static void SetCandidateAction(UPARAM(ref) FNsInteractionCandidate& Candidate, UPARAM(DisplayName = "Action") FNsInteractionExecuteDynamicDelegate Action);
};
