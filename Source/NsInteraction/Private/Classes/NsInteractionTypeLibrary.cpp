// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsInteractionTypeLibrary.h"

void FNsInteractionAction::Reset()
{
    Dynamic.Unbind();
    Native.Reset();
}

void FNsInteractionAction::BindNative(TFunction<bool(AActor*)> InNative)
{
    Dynamic.Unbind();
    Native = MoveTemp(InNative);
}

void FNsInteractionAction::BindBlueprint(const FNsInteractionExecuteDynamicDelegate& InDynamic)
{
    Native.Reset();
    Dynamic = InDynamic;
}

bool FNsInteractionAction::IsBound() const
{
    return Native.IsSet() || Dynamic.IsBound();
}

bool FNsInteractionAction::Execute(AActor* const Interactor) const
{
    if (Native.IsSet())
    {
        return Native(Interactor);
    }

    return Dynamic.IsBound() && Dynamic.Execute(Interactor);
}

FNsInteractionCandidate::FNsInteractionCandidate()
    : TargetActor(nullptr)
    , PromptText(FText::GetEmpty())
    , PromptWorldLocation(FVector::ZeroVector)
    , Priority(ENsInteractionPriority::Normal)
    , Action()
{
}

bool FNsInteractionCandidate::HasExecute() const
{
    return Action.IsBound();
}

bool FNsInteractionCandidate::TryExecute(AActor* const Interactor) const
{
    return Action.Execute(Interactor);
}

FNsResolvedInteraction::FNsResolvedInteraction()
    : bHasInteraction(false)
    , Candidate()
    , Candidates()
{
}

void UNsInteractionCandidateLibrary::SetCandidateAction(FNsInteractionCandidate& Candidate, FNsInteractionExecuteDynamicDelegate Action)
{
    Candidate.Action.BindBlueprint(Action);
}
