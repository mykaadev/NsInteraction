// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/PrimitiveComponent.h"
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
 * Input that runs a candidate.
 */
UENUM(BlueprintType)
enum class ENsInteractionInput : uint8
{
    /** A press runs the action at once */
    Press UMETA(DisplayName = "Press"),
    /** The interactor holds the input for HoldSeconds. A short press runs the tap action, when bound */
    Hold UMETA(DisplayName = "Hold")
};

/**
 * Machine that runs a candidate.
 */
UENUM(BlueprintType)
enum class ENsInteractionAuthority : uint8
{
    /** The action runs where the interaction starts */
    Local UMETA(DisplayName = "Local"),
    /** The action runs on the authority. A client asks for it through OnAuthorityRequest, and the game sends the request */
    Authority UMETA(DisplayName = "Authority")
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

    /** Part of the target that this candidate is for. The view trace sets it to the aimed part. Null means the whole actor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TObjectPtr<UPrimitiveComponent> TargetComponent;

    /** Optional short name of the target, shown above the prompt */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText TitleText;

    /** Text shown to the player when this candidate wins. On a disabled candidate it says why the action is not available */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText PromptText;

    /** False shows the prompt but runs nothing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bEnabled;

    /** Input that runs the action */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    ENsInteractionInput Input;

    /** Seconds of the hold. Used only by a hold candidate */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "s"))
    float HoldSeconds;

    /** Machine that runs the action */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    ENsInteractionAuthority Authority;

    /** World location used by UI systems to position the prompt. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FVector PromptWorldLocation;

    /** Priority bias used after distance/facing have been scored. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    ENsInteractionPriority Priority;

    /** Action run when this candidate wins */
    UPROPERTY(BlueprintReadWrite, Category = "Interaction")
    FNsInteractionAction Action;

    /** Optional action of a short press on a hold candidate. It always runs locally */
    UPROPERTY(BlueprintReadWrite, Category = "Interaction")
    FNsInteractionAction TapAction;

    FNsInteractionCandidate();

    /** Returns true when this candidate has an action bound */
    bool HasExecute() const;

    /** Runs the bound action, when present. Returns false when unbound or rejected */
    bool TryExecute(AActor* Interactor) const;

    /** Returns true when two candidates show the same prompt for the same part */
    bool ShowsSame(const FNsInteractionCandidate& Other) const;
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNsInteractionChangedDelegate, const FNsResolvedInteraction&, Interaction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNsInteractionAuthorityRequestDelegate, const FNsInteractionCandidate&, Candidate);

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

    /** Assigns the event that runs on a short press of a hold candidate */
    UFUNCTION(BlueprintCallable, Category = "Interaction", meta = (DisplayName = "Set Interaction Candidate Tap Action"))
    static void SetCandidateTapAction(UPARAM(ref) FNsInteractionCandidate& Candidate, UPARAM(DisplayName = "Action") FNsInteractionExecuteDynamicDelegate Action);
};
