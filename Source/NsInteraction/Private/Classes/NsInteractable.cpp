// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsInteractable.h"

void INsInteractable::OnEnter_Implementation(class AActor* const InActor)
{
    // Nothing :)
}

void INsInteractable::OnExit_Implementation(class AActor* const InActor)
{
    // Nothing :)
}

FVector INsInteractable::GetInteractionWorldLocation_Implementation() const
{
    const AActor* const InteractableActor = Cast<AActor>(_getUObject());
    return InteractableActor != nullptr ? InteractableActor->GetActorLocation() : FVector::ZeroVector;
}

void INsInteractable::OnInteract_Implementation(AActor* const InActor)
{
    FNsInteractionCandidate Candidate;
    if (GetInteractionCandidate_Implementation(InActor, Candidate))
    {
        Candidate.TryExecute(InActor);
    }
}

bool INsInteractable::GetInteractionCandidate_Implementation(AActor* const InActor, FNsInteractionCandidate& OutCandidate)
{
    return false;
}

bool INsInteractable::ExecuteInteractionCandidate_Implementation(AActor* const InActor, const FNsInteractionCandidate& Candidate)
{
    return false;
}
