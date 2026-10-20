// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsInteractionTypes.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

FNsInteractionTarget::FNsInteractionTarget(AActor* const InActor, UPrimitiveComponent* const InPart)
    : Actor(InActor)
    , Part(InPart)
{
}

bool FNsInteractionTarget::operator==(const FNsInteractionTarget& Other) const
{
    return Actor == Other.Actor && Part == Other.Part;
}

bool FNsInteractionTarget::IsValid() const
{
    return ::IsValid(Actor);
}

FVector FNsInteractionTarget::GetLocation() const
{
    if (::IsValid(Part))
    {
        return Part->Bounds.Origin;
    }
    return ::IsValid(Actor) ? Actor->GetActorLocation() : FVector::ZeroVector;
}

bool FNsInteractionOption::ShowsSame(const FNsInteractionOption& Other) const
{
    return Target == Other.Target
        && ActionId == Other.ActionId
        && bEnabled == Other.bEnabled
        && Input == Other.Input
        && Title.EqualTo(Other.Title)
        && Prompt.EqualTo(Other.Prompt);
}
