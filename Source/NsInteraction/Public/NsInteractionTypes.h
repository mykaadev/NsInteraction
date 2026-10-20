// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NsInteractionTypes.generated.h"

/** Input that runs an option */
UENUM(BlueprintType)
enum class ENsInteractionInput : uint8
{
    /** A press runs the action at once */
    Press,
    /** The interactor holds the input for Hold Seconds. A short press runs Tap Action Id, when it is set */
    Hold
};

/** Machine that runs an option */
UENUM(BlueprintType)
enum class ENsInteractionAuthority : uint8
{
    /** The action runs on the machine of the interactor */
    Local,
    /** The action runs on the server. A client fires On Server Request, and the game sends the request */
    Server
};

/** One thing that an interactor can use: an actor, and one part of it when the actor has more than one */
USTRUCT(BlueprintType)
struct NSINTERACTION_API FNsInteractionTarget
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    FNsInteractionTarget() = default;

    /** Constructor */
    FNsInteractionTarget(class AActor* InActor, class UPrimitiveComponent* InPart = nullptr);

    /** Returns true when both targets name the same actor and the same part */
    bool operator==(const FNsInteractionTarget& Other) const;

    /** Returns true when the actor is valid */
    bool IsValid() const;

    /** Returns the world point of the target: the center of the part, or the actor location */
    FVector GetLocation() const;

// Variables
public:
    /** Actor that implements NsInteractable */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TObjectPtr<class AActor> Actor;

    /** Part of the actor. Null means the whole actor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TObjectPtr<class UPrimitiveComponent> Part;
};

/** What an interactable offers to one interactor now: the texts, the input, and the action to run */
USTRUCT(BlueprintType)
struct NSINTERACTION_API FNsInteractionOption
{
    GENERATED_BODY()

// Functions
public:
    /** Returns true when two options show the same prompt for the same target */
    bool ShowsSame(const FNsInteractionOption& Other) const;

// Variables
public:
    /** Target of the option. The interaction component fills it before it asks the interactable */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FNsInteractionTarget Target;

    /** Name of the action. One interactable can offer different actions, and Execute Interaction reads this name */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FName ActionId;

    /** Action of a short press on a hold option. None means that a short press does nothing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FName TapActionId;

    /** Optional short name of the target, shown above the prompt */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText Title;

    /** Text of the action. On a disabled option it says why the action is not available */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText Prompt;

    /** False shows the prompt but runs nothing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    bool bEnabled = true;

    /** Input that runs the action */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    ENsInteractionInput Input = ENsInteractionInput::Press;

    /** Seconds of the hold. Only a hold option uses it */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", Units = "s"))
    float HoldSeconds = 0.5f;

    /** Machine that runs the action */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    ENsInteractionAuthority Authority = ENsInteractionAuthority::Local;

    /** World point where the game shows the prompt. Zero means the location of the target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FVector PromptLocation = FVector::ZeroVector;

    /** A higher priority wins over a nearer target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    int32 Priority = 0;
};
