// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsInteractable.h"

bool INsInteractable::GetInteractionOption_Implementation(AActor* const Interactor, UPrimitiveComponent* const Part, FNsInteractionOption& OutOption)
{
    return false;
}

bool INsInteractable::ExecuteInteraction_Implementation(AActor* const Interactor, const FNsInteractionOption& Option)
{
    return false;
}

void INsInteractable::OnInteractorEnter_Implementation(AActor* const Interactor)
{
}

void INsInteractable::OnInteractorExit_Implementation(AActor* const Interactor)
{
}
