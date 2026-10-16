// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsInteractionPromptWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UNsInteractionPromptWidget::ShowInteraction(const FNsResolvedInteraction& Interaction)
{
    if (!Interaction.bHasInteraction)
    {
        HidePrompt();
        return;
    }

    const FNsInteractionCandidate& Candidate = Interaction.Candidate;
    ShowPrompt(Candidate.TitleText, Candidate.PromptText, Candidate.bEnabled, Candidate.Input);
}

void UNsInteractionPromptWidget::ShowPrompt(const FText& Title, const FText& Prompt, const bool bEnabled, const ENsInteractionInput Input)
{
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (bHasShown && ShownTitle.EqualTo(Title) && ShownPrompt.EqualTo(Prompt) && bShownEnabled == bEnabled && ShownInput == Input)
    {
        return;
    }

    bHasShown = true;
    ShownTitle = Title;
    ShownPrompt = Prompt;
    bShownEnabled = bEnabled;
    ShownInput = Input;
    OnPromptShown(Title, Prompt, bEnabled, Input);
}

void UNsInteractionPromptWidget::HidePrompt()
{
    SetVisibility(ESlateVisibility::Collapsed);
    bHasShown = false;
    SetHoldProgress(0.f);
}

void UNsInteractionPromptWidget::SetKey(const FText& KeyName, UTexture2D* const KeyImage)
{
    if (ShownKeyName.EqualTo(KeyName) && ShownKeyImage == KeyImage)
    {
        return;
    }

    ShownKeyName = KeyName;
    ShownKeyImage = KeyImage;
    RefreshKey();
}

void UNsInteractionPromptWidget::SetHoldProgress(const float Progress)
{
    const float Clamped = FMath::Clamp(Progress, 0.f, 1.f);
    if (FMath::IsNearlyEqual(ShownHoldProgress, Clamped, 0.01f) && (Clamped > 0.f) == (ShownHoldProgress > 0.f))
    {
        return;
    }

    ShownHoldProgress = Clamped;
    OnHoldProgressChanged(Clamped);
}

void UNsInteractionPromptWidget::OnPromptShown_Implementation(const FText& Title, const FText& Prompt, const bool bEnabled, const ENsInteractionInput Input)
{
    if (IsValid(TitleText))
    {
        TitleText->SetText(Title);
        TitleText->SetVisibility(Title.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }

    if (IsValid(ActionText))
    {
        ActionText->SetText(Prompt);
    }

    RefreshKey();
}

void UNsInteractionPromptWidget::OnHoldProgressChanged_Implementation(const float Progress)
{
    if (IsValid(HoldProgressBar))
    {
        HoldProgressBar->SetPercent(Progress);
        HoldProgressBar->SetVisibility(Progress > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void UNsInteractionPromptWidget::RefreshKey()
{
    if (IsValid(KeyIcon))
    {
        KeyIcon->SetBrushFromTexture(ShownKeyImage);
        KeyIcon->SetVisibility(IsValid(ShownKeyImage) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }

    if (IsValid(KeyText))
    {
        KeyText->SetText(ShownKeyName);
        KeyText->SetVisibility(!ShownKeyName.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }

    if (IsValid(KeyBadge))
    {
        const bool bHasKey = !ShownKeyName.IsEmpty() || IsValid(ShownKeyImage);
        KeyBadge->SetVisibility(bShownEnabled && bHasKey ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}
