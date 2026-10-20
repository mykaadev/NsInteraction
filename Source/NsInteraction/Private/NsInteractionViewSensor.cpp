// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsInteractionViewSensor.h"

#include "NsInteractable.h"
#include "NsInteractionComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

UNsInteractionViewSensor::UNsInteractionViewSensor()
    : TraceDistanceCm(300.f)
    , TraceChannel(ECC_Visibility)
    , AssistAngleDeg(0.f)
    , AssistStickDeg(2.f)
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UNsInteractionViewSensor::BeginPlay()
{
    Super::BeginPlay();
    if (!IsValid(Interaction))
    {
        Interaction = GetOwner()->FindComponentByClass<UNsInteractionComponent>();
    }
}

void UNsInteractionViewSensor::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (IsControlledHere())
    {
        RefreshFocus();
    }
}

void UNsInteractionViewSensor::SetViewSource(USceneComponent* const InViewSource)
{
    ViewSource = InViewSource;
}

void UNsInteractionViewSensor::SetInteractionComponent(UNsInteractionComponent* const InInteraction)
{
    Interaction = InInteraction;
}

void UNsInteractionViewSensor::RefreshFocus()
{
    UWorld* const World = GetWorld();
    if (!IsValid(ViewSource) || !IsValid(Interaction) || World == nullptr)
    {
        return;
    }

    const FVector Start = ViewSource->GetComponentLocation();
    const FVector Direction = ViewSource->GetForwardVector();
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NsInteractionView), false, GetOwner());
    FHitResult Hit;
    World->LineTraceSingleByChannel(Hit, Start, Start + Direction * TraceDistanceCm, TraceChannel, QueryParams);

    // The exact aim wins. The aim assist runs only when the exact aim gives no option
    const UPrimitiveComponent* const LastPart = Focus.Part;
    FNsInteractionTarget NewFocus = FindFirstWithOption({ FNsInteractionTarget(Hit.GetActor(), Hit.GetComponent()) });
    if (!NewFocus.IsValid() && AssistAngleDeg > 0.f)
    {
        TArray<FNsInteractionTarget> AssistTargets;
        FindAssistTargets(Start, Direction, LastPart, AssistTargets);
        NewFocus = FindFirstWithOption(AssistTargets);
    }

    Focus = NewFocus;
    if (Focus.IsValid())
    {
        Interaction->SetTargets({ Focus });
    }
    else
    {
        Interaction->ClearTargets();
    }
}

bool UNsInteractionViewSensor::IsPartInView(const FVector& Start, const FVector& Direction, const UPrimitiveComponent* const Part, const float ExtraAngleDeg) const
{
    UWorld* const World = GetWorld();
    if (!IsValid(Part) || World == nullptr)
    {
        return false;
    }

    FHitResult Hit;
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NsInteractionView), false, GetOwner());
    if (World->LineTraceSingleByChannel(Hit, Start, Start + Direction.GetSafeNormal() * TraceDistanceCm, TraceChannel, QueryParams)
        && Hit.GetComponent() == Part)
    {
        return true;
    }

    // The stick angle counts, because the sensor keeps an aimed part up to that angle
    const float AssistDeg = AssistAngleDeg > 0.f ? AssistAngleDeg + AssistStickDeg : 0.f;
    const float MaxAngleDeg = AssistDeg + FMath::Max(0.f, ExtraAngleDeg);
    float AngleDeg = 0.f;
    return MaxAngleDeg > 0.f && MeasureViewAngle(Start, Direction, Part, AngleDeg) && AngleDeg <= MaxAngleDeg;
}

void UNsInteractionViewSensor::FindAssistTargets(const FVector& Start, const FVector& Direction, const UPrimitiveComponent* const StickyPart,
    TArray<FNsInteractionTarget>& OutTargets) const
{
    OutTargets.Reset();
    UWorld* const World = GetWorld();
    if (World == nullptr || TraceDistanceCm <= 0.f)
    {
        return;
    }

    TArray<FOverlapResult> Overlaps;
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NsInteractionViewAssist), false, GetOwner());
    World->OverlapMultiByChannel(Overlaps, Start, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(TraceDistanceCm), QueryParams);

    TArray<TPair<float, FNsInteractionTarget>> Scored;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        // Only parts that block the channel can take the exact aim, so only those parts count
        UPrimitiveComponent* const Part = Overlap.GetComponent();
        AActor* const Actor = Overlap.GetActor();
        if (!Overlap.bBlockingHit || Part == nullptr || Actor == nullptr || !Actor->Implements<UNsInteractable>()
            || Scored.ContainsByPredicate([Part](const TPair<float, FNsInteractionTarget>& Entry) { return Entry.Value.Part == Part; }))
        {
            continue;
        }

        const float StickDeg = Part == StickyPart ? AssistStickDeg : 0.f;
        float AngleDeg = 0.f;
        if (MeasureViewAngle(Start, Direction, Part, AngleDeg) && AngleDeg <= AssistAngleDeg + StickDeg)
        {
            Scored.Emplace(AngleDeg - StickDeg, FNsInteractionTarget(Actor, Part));
        }
    }

    Scored.StableSort([](const TPair<float, FNsInteractionTarget>& Left, const TPair<float, FNsInteractionTarget>& Right)
    {
        return Left.Key < Right.Key;
    });
    for (const TPair<float, FNsInteractionTarget>& Entry : Scored)
    {
        OutTargets.Add(Entry.Value);
    }
}

bool UNsInteractionViewSensor::MeasureViewAngle(const FVector& Start, const FVector& Direction, const UPrimitiveComponent* const Part, float& OutAngleDeg) const
{
    UWorld* const World = GetWorld();
    const FVector Forward = Direction.GetSafeNormal();
    if (World == nullptr || !IsValid(Part) || !Part->IsRegistered() || Forward.IsNearlyZero())
    {
        return false;
    }

    const FTransform& Transform = Part->GetComponentTransform();
    if (Transform.GetScale3D().GetAbsMin() < KINDA_SMALL_NUMBER)
    {
        return false;
    }

    // The search runs in the local space of the part, so the bounds box stays aligned with the part
    const FBox LocalBox = Part->CalcBounds(FTransform::Identity).GetBox();
    const FVector LocalStart = Transform.InverseTransformPosition(Start);
    const FVector LocalEnd = Transform.InverseTransformPosition(Start + Forward * TraceDistanceCm);

    // The distance from the view line to a box falls, then rises, so a ternary search finds the nearest point
    float Low = 0.f;
    float High = 1.f;
    for (int32 Step = 0; Step < 24; ++Step)
    {
        const float LowThird = Low + (High - Low) / 3.f;
        const float HighThird = High - (High - Low) / 3.f;
        if (LocalBox.ComputeSquaredDistanceToPoint(FMath::Lerp(LocalStart, LocalEnd, LowThird))
            <= LocalBox.ComputeSquaredDistanceToPoint(FMath::Lerp(LocalStart, LocalEnd, HighThird)))
        {
            High = HighThird;
        }
        else
        {
            Low = LowThird;
        }
    }

    const FVector Point = Transform.TransformPosition(LocalBox.GetClosestPointTo(FMath::Lerp(LocalStart, LocalEnd, (Low + High) * 0.5f)));
    const FVector ToPoint = Point - Start;
    const float DistanceCm = ToPoint.Size();
    if (DistanceCm > TraceDistanceCm)
    {
        return false;
    }

    OutAngleDeg = DistanceCm < KINDA_SMALL_NUMBER
        ? 0.f
        : FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, ToPoint / DistanceCm), -1.f, 1.f)));

    // Another actor in front of the part hides it
    FHitResult Blocker;
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NsInteractionViewAssistSight), false, GetOwner());
    return !World->LineTraceSingleByChannel(Blocker, Start, Point, TraceChannel, QueryParams)
        || Blocker.GetActor() == Part->GetOwner()
        || Blocker.Distance >= DistanceCm - BlockerSlackCm;
}

FNsInteractionTarget UNsInteractionViewSensor::FindFirstWithOption(const TArray<FNsInteractionTarget>& Candidates) const
{
    AActor* const Interactor = GetOwner();
    for (const FNsInteractionTarget& Candidate : Candidates)
    {
        FNsInteractionOption Option;
        if (Candidate.Actor != Interactor && UNsInteractionComponent::QueryOption(Interactor, Candidate, Option))
        {
            return Candidate;
        }
    }
    return FNsInteractionTarget();
}

bool UNsInteractionViewSensor::IsControlledHere() const
{
    const APawn* const Pawn = Cast<APawn>(GetOwner());
    return Pawn == nullptr || Pawn->IsLocallyControlled();
}
