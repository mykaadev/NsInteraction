// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsInteractionComponent.h"

#include "NsInteractable.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogNsInteraction, Log, All);

struct FNsScoredInteractionCandidate
{
    FNsInteractionCandidate Candidate;
    float DistanceCm = 0.f;
    float FacingScoreCm = 0.f;
    float FinalScore = 0.f;
    int32 StableOrder = INDEX_NONE;
};

static bool IsScoredCandidatePreferred(const FNsScoredInteractionCandidate& Candidate, const FNsScoredInteractionCandidate& CurrentBest)
{
    if (!FMath::IsNearlyEqual(Candidate.FinalScore, CurrentBest.FinalScore))
    {
        return Candidate.FinalScore < CurrentBest.FinalScore;
    }

    if (!FMath::IsNearlyEqual(Candidate.DistanceCm, CurrentBest.DistanceCm))
    {
        return Candidate.DistanceCm < CurrentBest.DistanceCm;
    }

    return Candidate.StableOrder < CurrentBest.StableOrder;
}

static bool IsControlledHere(const AActor* OwnerActor)
{
    const APawn* const Pawn = Cast<APawn>(OwnerActor);
    return Pawn == nullptr || Pawn->IsLocallyControlled();
}

UNsInteractionComponent::UNsInteractionComponent()
    : InteractionRadiusCm(120.f)
    , bUseViewTrace(false)
    , ViewTraceDistanceCm(300.f)
    , ViewTraceChannel(ECC_Visibility)
    , bLogInteractionResolution(false)
    , CloseIntentDistanceCm(35.f)
    , PriorityScoreStepCm(24.f)
    , FacingScoreWeightCm(32.f)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UNsInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
    SetComponentTickEnabled(bUseViewTrace);
}

void UNsInteractionComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bUseViewTrace && IsControlledHere(ResolveInteractionOwner()))
    {
        RefreshFromView();
    }

    if (HoldCandidate.IsSet() && GetHoldProgress() >= 1.f)
    {
        const FNsInteractionCandidate Candidate = HoldCandidate.GetValue();
        HoldCandidate.Reset();
        SetComponentTickEnabled(bUseViewTrace);
        RunCandidate(Candidate);
    }
}

void UNsInteractionComponent::SetViewSource(USceneComponent* const InViewSource)
{
    ViewSource = InViewSource;
}

void UNsInteractionComponent::RefreshFromView()
{
    FocusHit = FHitResult();
    UWorld* const World = GetWorld();
    if (IsValid(ViewSource) && World != nullptr)
    {
        const FVector Start = ViewSource->GetComponentLocation();
        const FVector End = Start + ViewSource->GetForwardVector() * ViewTraceDistanceCm;
        const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NsInteractionView), false, ResolveInteractionOwner());
        World->LineTraceSingleByChannel(FocusHit, Start, End, ViewTraceChannel, QueryParams);
    }

    TArray<AActor*> Targets;
    if (AActor* const FocusActor = FocusHit.GetActor())
    {
        Targets.Add(FocusActor);
    }

    SyncInteractionQueue(Targets);
    RefreshInteraction();
}

void UNsInteractionComponent::RefreshInteraction()
{
    const FNsResolvedInteraction Resolved = ResolveCurrentInteraction();
    const bool bChanged = Resolved.bHasInteraction != CurrentInteraction.bHasInteraction
        || (Resolved.bHasInteraction && !Resolved.Candidate.ShowsSame(CurrentInteraction.Candidate));
    CurrentInteraction = Resolved;
    if (!bChanged)
    {
        return;
    }

    // A hold ends when the player looks at something else
    if (HoldCandidate.IsSet() && !(Resolved.bHasInteraction && Resolved.Candidate.TargetActor == HoldCandidate->TargetActor
        && Resolved.Candidate.TargetComponent == HoldCandidate->TargetComponent))
    {
        HoldCandidate.Reset();
        SetComponentTickEnabled(bUseViewTrace);
    }

    OnInteractionChanged.Broadcast(CurrentInteraction);
}

UPrimitiveComponent* UNsInteractionComponent::GetFocusComponent() const
{
    return FocusHit.GetComponent();
}

UPrimitiveComponent* UNsInteractionComponent::GetFocusComponentOf(const AActor* const Interactor)
{
    const UNsInteractionComponent* const Component = IsValid(Interactor) ? Interactor->FindComponentByClass<UNsInteractionComponent>() : nullptr;
    return Component != nullptr ? Component->GetFocusComponent() : nullptr;
}

bool UNsInteractionComponent::BeginInteract()
{
    if (!bUseViewTrace)
    {
        RefreshInteraction();
    }

    if (!CurrentInteraction.bHasInteraction)
    {
        return false;
    }

    const FNsInteractionCandidate& Candidate = CurrentInteraction.Candidate;
    if (Candidate.Input == ENsInteractionInput::Hold)
    {
        HoldCandidate = Candidate;
        HoldStartSeconds = GetWorld()->GetTimeSeconds();
        SetComponentTickEnabled(true);
        return true;
    }

    RunCandidate(Candidate);
    return true;
}

void UNsInteractionComponent::EndInteract()
{
    if (!HoldCandidate.IsSet())
    {
        return;
    }

    // The hold did not finish, so the press was a tap
    const FNsInteractionCandidate Candidate = HoldCandidate.GetValue();
    HoldCandidate.Reset();
    SetComponentTickEnabled(bUseViewTrace);
    if (Candidate.TapAction.IsBound())
    {
        Candidate.TapAction.Execute(ResolveInteractionOwner());
    }
}

float UNsInteractionComponent::GetHoldProgress() const
{
    if (!HoldCandidate.IsSet() || GetWorld() == nullptr)
    {
        return 0.f;
    }

    return FMath::Clamp(static_cast<float>(GetWorld()->GetTimeSeconds() - HoldStartSeconds) / FMath::Max(HoldCandidate->HoldSeconds, KINDA_SMALL_NUMBER), 0.f, 1.f);
}

bool UNsInteractionComponent::InteractAsAuthority(AActor* const TargetActor, UPrimitiveComponent* const TargetComponent)
{
    // The candidate builds from the requested part, the same way it built on the client
    FocusHit = FHitResult();
    FocusHit.HitObjectHandle = FActorInstanceHandle(TargetActor);
    FocusHit.Component = TargetComponent;
    SyncInteractionQueue({ TargetActor });
    const FNsResolvedInteraction Resolved = ResolveCurrentInteraction();
    if (!Resolved.bHasInteraction || Resolved.Candidate.TargetActor != TargetActor || !Resolved.Candidate.bEnabled)
    {
        return false;
    }

    InteractWithResolvedInteraction(Resolved);
    return true;
}

void UNsInteractionComponent::RunCandidate(const FNsInteractionCandidate& Candidate)
{
    if (!Candidate.bEnabled)
    {
        return;
    }

    const AActor* const OwnerActor = ResolveInteractionOwner();
    if (Candidate.Authority == ENsInteractionAuthority::Authority && OwnerActor != nullptr && !OwnerActor->HasAuthority())
    {
        OnAuthorityRequest.Broadcast(Candidate);
        return;
    }

    FNsResolvedInteraction Resolved;
    Resolved.bHasInteraction = true;
    Resolved.Candidate = Candidate;
    InteractWithResolvedInteraction(Resolved);
}

void UNsInteractionComponent::AddToInteractionQueue(AActor* const InActor)
{
    AActor* const OwnerActor = ResolveInteractionOwner();
    if (InActor != nullptr && InActor != OwnerActor && InActor->Implements<UNsInteractable>())
    {
        if (!InteractionQueue.Contains(InActor))
        {
            InteractionQueue.Emplace(InActor);

            if (OwnerActor != nullptr)
            {
                NotifyInteractableEnter(InActor, OwnerActor);
            }
        }
    }
}

void UNsInteractionComponent::RemoveFromInteractionQueue(AActor* const InActor)
{
    if (InActor != nullptr && InActor->Implements<UNsInteractable>())
    {
        if (InteractionQueue.Contains(InActor))
        {
            if (AActor* const OwnerActor = ResolveInteractionOwner())
            {
                NotifyInteractableExit(InActor, OwnerActor);
            }

            InteractionQueue.Remove(InActor);
        }
    }
}

void UNsInteractionComponent::ClearInteractionQueue()
{
    if (AActor* const OwnerActor = ResolveInteractionOwner())
    {
        for (AActor* const QueuedActor : InteractionQueue)
        {
            if (QueuedActor != nullptr && QueuedActor->Implements<UNsInteractable>())
            {
                NotifyInteractableExit(QueuedActor, OwnerActor);
            }
        }
    }

    InteractionQueue.Reset();
}

void UNsInteractionComponent::SyncInteractionQueue(const TArray<AActor*>& NearbyActors)
{
    TSet<AActor*> NearbyInteractables;
    AActor* const OwnerActor = ResolveInteractionOwner();

    for (AActor* const NearbyActor : NearbyActors)
    {
        if (NearbyActor != nullptr && NearbyActor != OwnerActor && NearbyActor->Implements<UNsInteractable>())
        {
            NearbyInteractables.Add(NearbyActor);
        }
    }

    for (int32 QueueIndex = InteractionQueue.Num() - 1; QueueIndex >= 0; --QueueIndex)
    {
        AActor* const QueuedActor = InteractionQueue[QueueIndex];
        if (!IsValid(QueuedActor) || !QueuedActor->Implements<UNsInteractable>())
        {
            InteractionQueue.RemoveAt(QueueIndex);
            continue;
        }

        if (!NearbyInteractables.Contains(QueuedActor))
        {
            RemoveFromInteractionQueue(QueuedActor);
        }
    }

    for (AActor* const NearbyInteractable : NearbyInteractables)
    {
        AddToInteractionQueue(NearbyInteractable);
    }
}

void UNsInteractionComponent::HandleInteractionSensorBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AddToInteractionQueue(OtherActor);
}

void UNsInteractionComponent::HandleInteractionSensorEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    RemoveFromInteractionQueue(OtherActor);
}

FNsResolvedInteraction UNsInteractionComponent::ResolveCurrentInteraction() const
{
    TArray<FNsInteractionCandidate> Candidates;
    AActor* const OwnerActor = ResolveInteractionOwner();

    for (int32 ActorIndex = 0; ActorIndex < InteractionQueue.Num(); ++ActorIndex)
    {
        AActor* const QueuedActor = InteractionQueue[ActorIndex];
        if (!IsValid(QueuedActor) || !QueuedActor->Implements<UNsInteractable>())
        {
            continue;
        }

        FNsInteractionCandidate Candidate;
        if (GetInteractionCandidateFromActor(QueuedActor, OwnerActor, Candidate))
        {
            if (Candidate.TargetActor == nullptr)
            {
                Candidate.TargetActor = QueuedActor;
            }

            Candidates.Add(Candidate);
        }
    }

    const FNsResolvedInteraction ResolvedInteraction = ResolveInteractionCandidates(Candidates);
    if (bLogInteractionResolution)
    {
        UE_LOG(LogNsInteraction, Log, TEXT("NsInteraction on %s resolved %s from %d candidates."),
            *GetNameSafe(OwnerActor),
            ResolvedInteraction.bHasInteraction ? *GetNameSafe(ResolvedInteraction.Candidate.TargetActor) : TEXT("none"),
            Candidates.Num());
    }

    return ResolvedInteraction;
}

FNsResolvedInteraction UNsInteractionComponent::ResolveInteractionCandidates(const TArray<FNsInteractionCandidate>& Candidates) const
{
    FNsResolvedInteraction ResolvedInteraction;
    TArray<FNsScoredInteractionCandidate> ScoredCandidates;
    AActor* const OwnerActor = ResolveInteractionOwner();

    for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
    {
        const FNsInteractionCandidate& Candidate = Candidates[CandidateIndex];
        if (!IsValid(Candidate.TargetActor))
        {
            if (bLogInteractionResolution)
            {
                UE_LOG(LogNsInteraction, Log, TEXT("NsInteraction discarded candidate %d for owner %s because target is invalid."),
                    CandidateIndex,
                    *GetNameSafe(OwnerActor));
            }

            continue;
        }

        if (Candidate.PromptText.IsEmpty())
        {
            if (bLogInteractionResolution)
            {
                UE_LOG(LogNsInteraction, Log, TEXT("NsInteraction discarded candidate %d target %s for owner %s because prompt text is empty."),
                    CandidateIndex,
                    *GetNameSafe(Candidate.TargetActor),
                    *GetNameSafe(OwnerActor));
            }

            continue;
        }

        FNsScoredInteractionCandidate ScoredCandidate;
        ScoredCandidate.Candidate = Candidate;
        ScoredCandidate.DistanceCm = OwnerActor != nullptr
            ? FVector::Dist(OwnerActor->GetActorLocation(), ResolveCandidateInteractionLocation(Candidate))
            : FVector::Dist(FVector::ZeroVector, ResolveCandidateInteractionLocation(Candidate));
        ScoredCandidate.FacingScoreCm = CalculateFacingScore(OwnerActor, Candidate, FacingScoreWeightCm);
        ScoredCandidate.FinalScore = CalculateFinalScore(ScoredCandidate.DistanceCm, ScoredCandidate.FacingScoreCm, Candidate.Priority);
        ScoredCandidate.StableOrder = CandidateIndex;

        if (InteractionRadiusCm > 0.f && ScoredCandidate.DistanceCm > InteractionRadiusCm)
        {
            if (bLogInteractionResolution)
            {
                UE_LOG(LogNsInteraction, Log, TEXT("NsInteraction discarded candidate %d target %s for owner %s because distance %.2f exceeds radius %.2f at %s."),
                    CandidateIndex,
                    *GetNameSafe(Candidate.TargetActor),
                    *GetNameSafe(OwnerActor),
                    ScoredCandidate.DistanceCm,
                    InteractionRadiusCm,
                    *ResolveCandidateInteractionLocation(Candidate).ToCompactString());
            }

            continue;
        }

        ScoredCandidates.Add(ScoredCandidate);
    }

    const bool bHasCloseIntentCandidate = ScoredCandidates.ContainsByPredicate([this](const FNsScoredInteractionCandidate& Candidate)
    {
        return CloseIntentDistanceCm > 0.f && Candidate.DistanceCm <= CloseIntentDistanceCm;
    });

    if (bHasCloseIntentCandidate)
    {
        for (FNsScoredInteractionCandidate& Candidate : ScoredCandidates)
        {
            if (Candidate.DistanceCm > CloseIntentDistanceCm)
            {
                Candidate.FinalScore += 100000.f;
            }
        }
    }

    ScoredCandidates.Sort([](const FNsScoredInteractionCandidate& LeftCandidate, const FNsScoredInteractionCandidate& RightCandidate)
    {
        return IsScoredCandidatePreferred(LeftCandidate, RightCandidate);
    });

    for (const FNsScoredInteractionCandidate& ScoredCandidate : ScoredCandidates)
    {
        ResolvedInteraction.Candidates.Add(ScoredCandidate.Candidate);
    }

    ResolvedInteraction.bHasInteraction = !ScoredCandidates.IsEmpty();
    if (ResolvedInteraction.bHasInteraction)
    {
        ResolvedInteraction.Candidate = ScoredCandidates[0].Candidate;
    }

    return ResolvedInteraction;
}

bool UNsInteractionComponent::IsActorInInteractionRange(AActor* const InActor) const
{
    const AActor* const OwnerActor = ResolveInteractionOwner();
    if (!IsValid(InActor) || OwnerActor == nullptr || InteractionRadiusCm <= 0.f)
    {
        return false;
    }

    return FVector::DistSquared(OwnerActor->GetActorLocation(), ResolveActorInteractionLocation(InActor)) <= FMath::Square(InteractionRadiusCm);
}

void UNsInteractionComponent::Interact()
{
    InteractResolved();
}

void UNsInteractionComponent::InteractResolved()
{
    InteractWithResolvedInteraction(ResolveCurrentInteraction());
}

void UNsInteractionComponent::InteractWithResolvedInteraction(const FNsResolvedInteraction& ResolvedInteraction)
{
    if (!ResolvedInteraction.bHasInteraction || !ResolvedInteraction.Candidate.bEnabled)
    {
        return;
    }

    AActor* const TargetActor = ResolvedInteraction.Candidate.TargetActor;
    AActor* const OwnerActor = ResolveInteractionOwner();
    const FVector CandidateLocation = ResolveCandidateInteractionLocation(ResolvedInteraction.Candidate);
    const bool bCandidateInRange = InteractionRadiusCm <= 0.f
        || (OwnerActor != nullptr && FVector::DistSquared(OwnerActor->GetActorLocation(), CandidateLocation) <= FMath::Square(InteractionRadiusCm));

    if (!InteractionQueue.Contains(TargetActor))
    {
        UE_LOG(LogNsInteraction, Error, TEXT("Resolved interaction target %s is no longer queued for owner %s. QueueCount=%d"),
            *GetNameSafe(TargetActor),
            *GetNameSafe(OwnerActor),
            InteractionQueue.Num());
        return;
    }

    if (!bCandidateInRange)
    {
        const float DistanceCm = OwnerActor != nullptr ? FVector::Dist(OwnerActor->GetActorLocation(), CandidateLocation) : -1.f;
        UE_LOG(LogNsInteraction, Error, TEXT("Resolved interaction target %s is outside range for owner %s. Distance=%.2f Radius=%.2f CandidateLocation=%s"),
            *GetNameSafe(TargetActor),
            *GetNameSafe(OwnerActor),
            DistanceCm,
            InteractionRadiusCm,
            *CandidateLocation.ToCompactString());
        return;
    }

    if (OwnerActor == nullptr)
    {
        UE_LOG(LogNsInteraction, Error, TEXT("Cannot execute resolved interaction for target %s because interaction owner is null."), *GetNameSafe(TargetActor));
        return;
    }

    if (TargetActor == nullptr || !TargetActor->Implements<UNsInteractable>())
    {
        UE_LOG(LogNsInteraction, Error, TEXT("Cannot execute resolved interaction for owner %s because target %s is not a valid interactable."), *GetNameSafe(OwnerActor), *GetNameSafe(TargetActor));
        return;
    }

    if (ResolvedInteraction.Candidate.HasExecute())
    {
        if (ResolvedInteraction.Candidate.TryExecute(OwnerActor))
        {
            return;
        }

        UE_LOG(LogNsInteraction, Error, TEXT("Resolved interaction action on target %s failed for owner %s."), *GetNameSafe(TargetActor), *GetNameSafe(OwnerActor));
        return;
    }

    if (ExecuteInteractionCandidateOnActor(TargetActor, OwnerActor, ResolvedInteraction.Candidate))
    {
        return;
    }

    NotifyInteractableUsed(TargetActor, OwnerActor);
}

AActor* UNsInteractionComponent::ResolveInteractionOwner() const
{
    if (AActor* const OwnerActor = GetOwner())
    {
        return OwnerActor;
    }

    return GetTypedOuter<AActor>();
}

void UNsInteractionComponent::NotifyInteractableEnter(AActor* const InteractableActor, AActor* const OwnerActor)
{
    if (InteractableActor == nullptr || !InteractableActor->Implements<UNsInteractable>())
    {
        return;
    }

    if (INsInteractable* const NativeInteractable = Cast<INsInteractable>(InteractableActor))
    {
        NativeInteractable->OnEnter_Implementation(OwnerActor);
        return;
    }

    INsInteractable::Execute_OnEnter(InteractableActor, OwnerActor);
}

void UNsInteractionComponent::NotifyInteractableExit(AActor* const InteractableActor, AActor* const OwnerActor)
{
    if (InteractableActor == nullptr || !InteractableActor->Implements<UNsInteractable>())
    {
        return;
    }

    if (INsInteractable* const NativeInteractable = Cast<INsInteractable>(InteractableActor))
    {
        NativeInteractable->OnExit_Implementation(OwnerActor);
        return;
    }

    INsInteractable::Execute_OnExit(InteractableActor, OwnerActor);
}

void UNsInteractionComponent::NotifyInteractableUsed(AActor* const InteractableActor, AActor* const OwnerActor)
{
    if (InteractableActor == nullptr || !InteractableActor->Implements<UNsInteractable>())
    {
        return;
    }

    if (INsInteractable* const NativeInteractable = Cast<INsInteractable>(InteractableActor))
    {
        NativeInteractable->OnInteract_Implementation(OwnerActor);
        return;
    }

    INsInteractable::Execute_OnInteract(InteractableActor, OwnerActor);
}

bool UNsInteractionComponent::GetInteractionCandidateFromActor(AActor* const InteractableActor, AActor* const OwnerActor, FNsInteractionCandidate& OutCandidate)
{
    if (InteractableActor == nullptr || !InteractableActor->Implements<UNsInteractable>())
    {
        return false;
    }

    if (INsInteractable* const NativeInteractable = Cast<INsInteractable>(InteractableActor))
    {
        return NativeInteractable->GetInteractionCandidate_Implementation(OwnerActor, OutCandidate);
    }

    return INsInteractable::Execute_GetInteractionCandidate(InteractableActor, OwnerActor, OutCandidate);
}

bool UNsInteractionComponent::ExecuteInteractionCandidateOnActor(AActor* const InteractableActor, AActor* const OwnerActor, const FNsInteractionCandidate& Candidate)
{
    if (InteractableActor == nullptr || !InteractableActor->Implements<UNsInteractable>())
    {
        return false;
    }

    if (INsInteractable* const NativeInteractable = Cast<INsInteractable>(InteractableActor))
    {
        return NativeInteractable->ExecuteInteractionCandidate_Implementation(OwnerActor, Candidate);
    }

    return INsInteractable::Execute_ExecuteInteractionCandidate(InteractableActor, OwnerActor, Candidate);
}

FVector UNsInteractionComponent::ResolveActorInteractionLocation(AActor* const InActor)
{
    if (InActor == nullptr)
    {
        return FVector::ZeroVector;
    }

    const USceneComponent* const RootComponent = InActor->GetRootComponent();
    return RootComponent != nullptr && RootComponent->IsRegistered()
        ? RootComponent->GetComponentLocation()
        : InActor->GetActorLocation();
}

FVector UNsInteractionComponent::ResolveCandidateInteractionLocation(const FNsInteractionCandidate& Candidate)
{
    return !Candidate.PromptWorldLocation.IsNearlyZero()
        ? Candidate.PromptWorldLocation
        : ResolveActorInteractionLocation(Candidate.TargetActor);
}

float UNsInteractionComponent::CalculateFacingScore(AActor* const OwnerActor, const FNsInteractionCandidate& Candidate, const float FacingWeightCm)
{
    if (OwnerActor == nullptr || Candidate.TargetActor == nullptr || FacingWeightCm <= 0.f)
    {
        return 0.f;
    }

    const FVector ToCandidate = (ResolveCandidateInteractionLocation(Candidate) - OwnerActor->GetActorLocation()).GetSafeNormal2D();
    if (ToCandidate.IsNearlyZero())
    {
        return 0.f;
    }

    const FVector OwnerForward = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
    if (OwnerForward.IsNearlyZero())
    {
        return 0.f;
    }

    const float FacingDot = FMath::Clamp(FVector::DotProduct(OwnerForward, ToCandidate), -1.f, 1.f);
    return (1.f - FacingDot) * FacingWeightCm;
}

float UNsInteractionComponent::CalculateFinalScore(const float DistanceCm, const float FacingScoreCm, const ENsInteractionPriority Priority) const
{
    const float DistanceScore = FMath::Max(0.f, DistanceCm);
    const float FacingScore = FMath::Max(0.f, FacingScoreCm);
    if (CloseIntentDistanceCm > 0.f && DistanceScore <= CloseIntentDistanceCm)
    {
        return DistanceScore + FacingScore;
    }

    const int32 PriorityDelta = GetPriorityValue(Priority) - GetPriorityValue(ENsInteractionPriority::Normal);
    return DistanceScore + FacingScore - static_cast<float>(PriorityDelta) * PriorityScoreStepCm;
}

int32 UNsInteractionComponent::GetPriorityValue(const ENsInteractionPriority Priority)
{
    switch (Priority)
    {
        case ENsInteractionPriority::Low:
            return 0;
        case ENsInteractionPriority::High:
            return 2;
        case ENsInteractionPriority::Critical:
            return 3;
        case ENsInteractionPriority::Normal:
        default:
            return 1;
    }
}
