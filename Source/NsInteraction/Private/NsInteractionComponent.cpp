// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsInteractionComponent.h"

#include "NsInteractable.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UNsInteractionComponent::UNsInteractionComponent()
    : RefreshInterval(0.f)
    , MaxDistanceCm(0.f)
    , bHasOption(false)
    , HoldStartSeconds(0.0)
    , RefreshWait(0.f)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UNsInteractionComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (RefreshInterval > 0.f && !Targets.IsEmpty())
    {
        RefreshWait -= DeltaTime;
        if (RefreshWait <= 0.f)
        {
            RefreshWait = RefreshInterval;
            Refresh();
        }
    }

    if (HoldOption.IsSet() && GetHoldProgress() >= 1.f)
    {
        const FNsInteractionOption Held = HoldOption.GetValue();
        HoldOption.Reset();
        UpdateTickState();
        RunAction(Held, Held.ActionId);
    }
}

void UNsInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearTargets();
    Super::EndPlay(EndPlayReason);
}

void UNsInteractionComponent::AddTarget(AActor* const Actor, UPrimitiveComponent* const Part)
{
    const FNsInteractionTarget Target(Actor, Part);
    if (!Target.IsValid() || Targets.Contains(Target))
    {
        return;
    }

    TArray<FNsInteractionTarget> NewTargets = Targets;
    NewTargets.Add(Target);
    SetTargets(NewTargets);
}

void UNsInteractionComponent::RemoveTarget(AActor* const Actor)
{
    TArray<FNsInteractionTarget> NewTargets = Targets;
    if (NewTargets.RemoveAll([Actor](const FNsInteractionTarget& Target) { return Target.Actor == Actor; }) > 0)
    {
        SetTargets(NewTargets);
    }
}

void UNsInteractionComponent::SetTargets(const TArray<FNsInteractionTarget>& NewTargets)
{
    // An actor gets one enter when its first target comes, and one exit when its last target goes
    AActor* const Interactor = GetOwner();
    TSet<AActor*> OldActors;
    for (const FNsInteractionTarget& Target : Targets)
    {
        OldActors.Add(Target.Actor);
    }

    TSet<AActor*> NewActors;
    Targets.Reset();
    for (const FNsInteractionTarget& Target : NewTargets)
    {
        if (Target.IsValid() && Target.Actor != Interactor && !Targets.Contains(Target))
        {
            Targets.Add(Target);
            NewActors.Add(Target.Actor);
        }
    }

    for (AActor* const Actor : OldActors)
    {
        if (IsValid(Actor) && !NewActors.Contains(Actor) && Actor->Implements<UNsInteractable>())
        {
            INsInteractable::Execute_OnInteractorExit(Actor, Interactor);
        }
    }

    for (AActor* const Actor : NewActors)
    {
        if (!OldActors.Contains(Actor) && Actor->Implements<UNsInteractable>())
        {
            INsInteractable::Execute_OnInteractorEnter(Actor, Interactor);
        }
    }

    UpdateTickState();
    Refresh();
}

void UNsInteractionComponent::ClearTargets()
{
    if (!Targets.IsEmpty())
    {
        SetTargets({});
    }
}

void UNsInteractionComponent::HandleSensorBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp,
    const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
    AddTarget(OtherActor);
}

void UNsInteractionComponent::HandleSensorEndOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp,
    const int32 OtherBodyIndex)
{
    RemoveTarget(OtherActor);
}

void UNsInteractionComponent::Refresh()
{
    AActor* const Interactor = GetOwner();
    bool bFound = false;
    float BestScore = 0.f;
    FNsInteractionOption Best;
    for (const FNsInteractionTarget& Target : Targets)
    {
        FNsInteractionOption Candidate;
        if (!IsInReach(Target) || !QueryOption(Interactor, Target, Candidate))
        {
            continue;
        }

        // The first target wins a tie, so a sensor can give its targets in order
        const float Score = ScoreOption(Candidate);
        if (!bFound || Score > BestScore)
        {
            bFound = true;
            BestScore = Score;
            Best = Candidate;
        }
    }

    const bool bChanged = bFound != bHasOption || (bFound && !Best.ShowsSame(Option));
    bHasOption = bFound;
    Option = bFound ? Best : FNsInteractionOption();
    if (!bChanged)
    {
        return;
    }

    // A hold ends when the target changes
    if (HoldOption.IsSet() && !(bHasOption && Option.Target == HoldOption->Target))
    {
        CancelHold();
    }

    OnOptionChanged.Broadcast(bHasOption, Option);
}

bool UNsInteractionComponent::BeginInteract()
{
    Refresh();
    if (!bHasOption)
    {
        return false;
    }

    if (Option.Input == ENsInteractionInput::Hold)
    {
        HoldOption = Option;
        HoldStartSeconds = GetWorld()->GetTimeSeconds();
        UpdateTickState();
        return true;
    }

    RunAction(Option, Option.ActionId);
    return true;
}

void UNsInteractionComponent::EndInteract()
{
    if (!HoldOption.IsSet())
    {
        return;
    }

    // The hold did not finish, so the press was a tap
    const FNsInteractionOption Held = HoldOption.GetValue();
    CancelHold();
    if (!Held.TapActionId.IsNone())
    {
        RunAction(Held, Held.TapActionId);
    }
}

float UNsInteractionComponent::GetHoldProgress() const
{
    const UWorld* const World = GetWorld();
    if (!HoldOption.IsSet() || World == nullptr)
    {
        return 0.f;
    }

    const float Elapsed = static_cast<float>(World->GetTimeSeconds() - HoldStartSeconds);
    return FMath::Clamp(Elapsed / FMath::Max(HoldOption->HoldSeconds, KINDA_SMALL_NUMBER), 0.f, 1.f);
}

bool UNsInteractionComponent::ExecuteOnServer(AActor* const Actor, UPrimitiveComponent* const Part, const FName ActionId)
{
    // The option builds again on the server from the same target, so the server state decides
    AActor* const Interactor = GetOwner();
    const FNsInteractionTarget Target(Actor, Part);
    FNsInteractionOption Candidate;
    if (!IsInReach(Target) || !QueryOption(Interactor, Target, Candidate) || !Candidate.bEnabled
        || (ActionId != Candidate.ActionId && ActionId != Candidate.TapActionId))
    {
        return false;
    }

    Candidate.ActionId = ActionId;
    return INsInteractable::Execute_ExecuteInteraction(Actor, Interactor, Candidate);
}

bool UNsInteractionComponent::QueryOption(AActor* const Interactor, const FNsInteractionTarget& Target, FNsInteractionOption& OutOption)
{
    if (!Target.IsValid() || !Target.Actor->Implements<UNsInteractable>())
    {
        return false;
    }

    OutOption = FNsInteractionOption();
    OutOption.Target = Target;
    if (!INsInteractable::Execute_GetInteractionOption(Target.Actor, Interactor, Target.Part, OutOption))
    {
        return false;
    }

    // The interactable can name a part. Without one, the option keeps the part of the target
    OutOption.Target.Actor = Target.Actor;
    if (!IsValid(OutOption.Target.Part))
    {
        OutOption.Target.Part = Target.Part;
    }

    if (OutOption.PromptLocation.IsZero())
    {
        OutOption.PromptLocation = OutOption.Target.GetLocation();
    }
    return true;
}

float UNsInteractionComponent::ScoreOption_Implementation(const FNsInteractionOption& Candidate) const
{
    // One priority step is worth more than any distance in reach
    constexpr float PriorityStep = 100000.f;
    const AActor* const Interactor = GetOwner();
    const float Distance = IsValid(Interactor) ? FVector::Dist(Interactor->GetActorLocation(), Candidate.Target.GetLocation()) : 0.f;
    return Candidate.Priority * PriorityStep - Distance;
}

void UNsInteractionComponent::RunAction(const FNsInteractionOption& Candidate, const FName ActionId)
{
    if (!Candidate.bEnabled || !Candidate.Target.IsValid())
    {
        return;
    }

    FNsInteractionOption Request = Candidate;
    Request.ActionId = ActionId;
    const AActor* const Interactor = GetOwner();
    if (Candidate.Authority == ENsInteractionAuthority::Server && IsValid(Interactor) && !Interactor->HasAuthority())
    {
        OnServerRequest.Broadcast(Request);
        return;
    }

    INsInteractable::Execute_ExecuteInteraction(Candidate.Target.Actor, GetOwner(), Request);
}

bool UNsInteractionComponent::IsInReach(const FNsInteractionTarget& Target) const
{
    const AActor* const Interactor = GetOwner();
    return MaxDistanceCm <= 0.f || !IsValid(Interactor)
        || FVector::Dist(Interactor->GetActorLocation(), Target.GetLocation()) <= MaxDistanceCm;
}

void UNsInteractionComponent::CancelHold()
{
    HoldOption.Reset();
    UpdateTickState();
}

void UNsInteractionComponent::UpdateTickState()
{
    SetComponentTickEnabled(HoldOption.IsSet() || (RefreshInterval > 0.f && !Targets.IsEmpty()));
}
