// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NsInteractionTypes.h"
#include "NsInteractionViewSensor.generated.h"

/**
 * Sensor for first-person aim. Each frame on the machine that controls the owner, it traces along the view
 * and gives the aimed part to the interaction component as its only target
 * The exact aim wins. When the exact aim gives no option, an optional aim assist takes the part nearest to the view direction
 */
UCLASS(ClassGroup = (NsInteraction), meta = (BlueprintSpawnableComponent))
class NSINTERACTION_API UNsInteractionViewSensor : public UActorComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsInteractionViewSensor();

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    /** Sets the component that the trace starts from and points along. Begin Play uses the first camera of the owner when none is set */
    UFUNCTION(BlueprintCallable, Category = "Interaction|View")
    void SetViewSource(class USceneComponent* InViewSource);

    /** Sets the interaction component that gets the aimed target. Begin Play finds one on the owner when none is set */
    UFUNCTION(BlueprintCallable, Category = "Interaction|View")
    void SetInteractionComponent(class UNsInteractionComponent* InInteraction);

    /** Returns the aimed target. The actor is null when the aim finds nothing */
    UFUNCTION(BlueprintPure, Category = "Interaction|View")
    const FNsInteractionTarget& GetFocus() const { return Focus; }

    /** Returns true when a view from Start along Direction aims at the part, with the same rule as the sensor. Extra Angle Deg widens the cone, for example for network lag on the server */
    UFUNCTION(BlueprintCallable, Category = "Interaction|View")
    bool IsPartInView(const FVector& Start, const FVector& Direction, const class UPrimitiveComponent* Part, float ExtraAngleDeg = 0.f) const;

    /** Traces from the view source and gives the aimed target to the interaction component. The tick calls it */
    UFUNCTION(BlueprintCallable, Category = "Interaction|View")
    void RefreshFocus();

private:
    /** Collects the interactable parts inside the assist cone, nearest to the view direction first. The sticky part gets the stick angle */
    void FindAssistTargets(const FVector& Start, const FVector& Direction, const class UPrimitiveComponent* StickyPart, TArray<FNsInteractionTarget>& OutTargets) const;

    /** Measures the angle from the view direction to the bounds of a part. Returns false when the part is out of reach or behind another actor */
    bool MeasureViewAngle(const FVector& Start, const FVector& Direction, const class UPrimitiveComponent* Part, float& OutAngleDeg) const;

    /** Returns the first target that gives an option, or an empty target */
    FNsInteractionTarget FindFirstWithOption(const TArray<FNsInteractionTarget>& Candidates) const;

    /** Returns true when the owner is a pawn that this machine controls, or not a pawn */
    bool IsControlledHere() const;

// Variables
public:
    /** Length of the trace */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|View", meta = (ClampMin = "0.0", Units = "cm"))
    float TraceDistanceCm;

    /** Channel of the trace. A part needs to block it to take the aim */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|View")
    TEnumAsByte<ECollisionChannel> TraceChannel;

    /** Half-angle of the assist cone. 0 turns the aim assist off */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|View", meta = (ClampMin = "0.0", ClampMax = "45.0", Units = "Degrees"))
    float AssistAngleDeg;

    /** The assist keeps the aimed part while no other part is nearer to the view direction by more than this angle. It stops a flicker between two close parts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|View", meta = (ClampMin = "0.0", Units = "Degrees"))
    float AssistStickDeg;

private:
    /** A blocker this close in front of the measured point does not hide the part, because the point is on the bounds box and not on the collision */
    static constexpr float BlockerSlackCm = 10.f;

    /** Start of the trace */
    UPROPERTY(Transient)
    TObjectPtr<class USceneComponent> ViewSource;

    /** Component that gets the aimed target */
    UPROPERTY(Transient)
    TObjectPtr<class UNsInteractionComponent> Interaction;

    /** Aimed target of the last refresh */
    UPROPERTY(Transient)
    FNsInteractionTarget Focus;
};
