// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NsInteractionTypeLibrary.h"
#include "NsInteractionPromptWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UTexture2D;

/**
 * Base of an interaction prompt: a title, an action line, a key badge, and a hold bar
 * A Widget Blueprint based on it sets the look. Every bound part is optional
 * Override OnPromptShown and OnHoldProgressChanged to add more style, such as a color for a disabled prompt
 */
UCLASS(Abstract, Blueprintable)
class NSINTERACTION_API UNsInteractionPromptWidget : public UUserWidget
{
    GENERATED_BODY()

// Functions
public:

    /** Shows the winning candidate, or hides the prompt when there is none */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void ShowInteraction(const FNsResolvedInteraction& Interaction);

    /** Shows a prompt. A prompt that does not change does nothing */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void ShowPrompt(const FText& Title, const FText& Prompt, bool bEnabled, ENsInteractionInput Input);

    /** Hides the prompt */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void HidePrompt();

    /** Sets the key hint of the badge. The game reads it from its input system. An icon replaces the text */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetKey(const FText& KeyName, UTexture2D* KeyImage);

    /** Fills the hold bar, from 0 to 1. 0 hides the bar */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetHoldProgress(float Progress);

protected:

    /** Writes the prompt into the bound parts after a change */
    UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
    void OnPromptShown(const FText& Title, const FText& Prompt, bool bEnabled, ENsInteractionInput Input);
    virtual void OnPromptShown_Implementation(const FText& Title, const FText& Prompt, bool bEnabled, ENsInteractionInput Input);

    /** Writes the hold progress into the bar after a change */
    UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
    void OnHoldProgressChanged(float Progress);
    virtual void OnHoldProgressChanged_Implementation(float Progress);

    /** Shows the key badge for an enabled prompt with a key text or a key icon */
    void RefreshKey();

// Variables
protected:

    /** Optional short name of the target. It hides when the title is empty */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TitleText;

    /** Optional action text, or the reason when the action is not available */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ActionText;

    /** Optional badge around the key hint */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidgetOptional))
    TObjectPtr<UWidget> KeyBadge;

    /** Optional key text */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> KeyText;

    /** Optional key icon. The key text shows when the icon is missing */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidgetOptional))
    TObjectPtr<UImage> KeyIcon;

    /** Optional bar that fills during a hold */
    UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> HoldProgressBar;

    /** Prompt on screen */
    FText ShownTitle;
    FText ShownPrompt;
    bool bShownEnabled = false;
    ENsInteractionInput ShownInput = ENsInteractionInput::Press;
    bool bHasShown = false;

    /** Key hint on screen */
    FText ShownKeyName;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> ShownKeyImage;

    /** Hold progress on screen */
    float ShownHoldProgress = 0.f;
};
