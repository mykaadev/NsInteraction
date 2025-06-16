// Copyright (C) 2024 mykaa. All rights reserved.

#include "NsInteractionComponent.h"
#include "NsInteractable.h"

UNsInteractionComponent::UNsInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UNsInteractionComponent::AddToInteractionQueue(class AActor* const InActor)
{
    if (InActor != nullptr && InActor->Implements<UNsInteractable>())
    {
        if (!InteractionQueue.Contains(InActor))
        {
            InteractionQueue.Emplace(InActor);

            if (AActor* const OwnerActor = GetOwner())
            {
                INsInteractable::Execute_OnEnter(InActor, OwnerActor);
            }
        }
    }
}

void UNsInteractionComponent::RemoveFromInteractionQueue(class AActor* const InActor)
{
    if (InActor != nullptr && InActor->Implements<UNsInteractable>())
    {
        if (InteractionQueue.Contains(InActor))
        {
            if (AActor* const OwnerActor = GetOwner())
            {
                INsInteractable::Execute_OnExit(InActor, OwnerActor);
            }

            InteractionQueue.Remove(InActor);
        }
    }
}

AActor* UNsInteractionComponent::GetCurrentInteractableActor()
{
    return !InteractionQueue.IsEmpty() ? InteractionQueue.Last() : nullptr;
}

void UNsInteractionComponent::Interact()
{
    if (!InteractionQueue.IsEmpty())
    {
        AActor* const OwnerActor = GetOwner();
        AActor* const CurrentActor = GetCurrentInteractableActor();

        if (OwnerActor != nullptr && CurrentActor != nullptr && CurrentActor->Implements<UNsInteractable>())
        {
            INsInteractable::Execute_OnInteract(CurrentActor, OwnerActor);
        }
    }
}