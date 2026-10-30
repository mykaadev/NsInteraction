// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsInteractableActor.h"

#include "Components/SceneComponent.h"

ANsInteractableActor::ANsInteractableActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    Option.ActionId = TEXT("Use");
    Option.Prompt = NSLOCTEXT("NsInteraction", "DefaultPrompt", "Use");
    Option.Authority = ENsInteractionAuthority::Server;
}

bool ANsInteractableActor::GetInteractionOption_Implementation(AActor* const Interactor, UPrimitiveComponent* const Part, FNsInteractionOption& OutOption)
{
    // The target comes from the interaction component. The other fields come from the details panel
    const FNsInteractionTarget Target = OutOption.Target;
    OutOption = Option;
    OutOption.Target = Target;
    return true;
}

bool ANsInteractableActor::ExecuteInteraction_Implementation(AActor* const Interactor, const FNsInteractionOption& InOption)
{
    ReceiveInteract(Interactor, InOption.ActionId);
    OnInteracted.Broadcast(Interactor, InOption.ActionId);
    return true;
}

void ANsInteractableActor::SetInteractionEnabled(const bool bEnabled)
{
    Option.bEnabled = bEnabled;
}
