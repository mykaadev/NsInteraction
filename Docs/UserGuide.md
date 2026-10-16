# NsInteraction User Guide

## Purpose

NsInteraction lets an actor, usually the player, interact with nearby actors. It keeps a queue of nearby interactables, picks the best one by distance, facing, and priority, and runs its action.

It also has an optional view trace for first-person aim, a press or hold input, and a prompt widget base that a Widget Blueprint styles.

NsInteraction does not replicate. An action that needs the server sends a request event, and the game sends the server call.

## Setup

1. Add `NsInteraction` to the `Plugins` list of the `.uproject` file.
2. For C++, add `"NsInteraction"` to the module dependencies in `Build.cs`.

## Quick Start

### 1. The interactor

1. Add a `NsInteractionComponent` to the player pawn.
2. Pick a sensor:
   - Aim: turn on `bUseViewTrace` and call `SetViewSource` with the camera. The component traces every frame on the controlling machine.
   - Nearby: add a sphere or box collision component. Bind its overlap events to `HandleInteractionSensorBeginOverlap` and `HandleInteractionSensorEndOverlap`.
3. Call `BeginInteract` when the interact input starts and `EndInteract` when it ends. `Interact` still works for a press-only setup.

### 2. The interactable

Implement the `NsInteractable` interface on an actor. Override `GetInteractionCandidate`.

```cpp
bool AMySlotMachine::GetInteractionCandidate_Implementation(AActor* const InActor, FNsInteractionCandidate& OutCandidate)
{
    OutCandidate.TargetActor = this;
    OutCandidate.PromptText = NSLOCTEXT("SlotMachine", "Play", "Play");
    OutCandidate.Action.BindNative([this](AActor* const Interactor)
    {
        return TryPlay(Interactor);
    });
    return true;
}
```

In Blueprint, fill the candidate, then call `Set Interaction Candidate Action` with a custom event.

### 3. The prompt

1. Make a Widget Blueprint based on `NsInteractionPromptWidget`.
2. Add any of these parts with these names: `TitleText`, `ActionText`, `KeyBadge`, `KeyText`, `KeyIcon`, `HoldProgressBar`. Each part is optional.
3. Bind `OnInteractionChanged` on the component to `ShowInteraction` on the widget.
4. Each frame of a hold, call `SetHoldProgress` with `GetHoldProgress`.
5. Call `SetKey` with the key text or icon from your input system.
6. Override `OnPromptShown` and `OnHoldProgressChanged` to add a style, such as a grey color for a disabled prompt.

## Common Tasks

### Show a prompt without the widget base

1. With the view trace, read `GetCurrentInteraction` or bind `OnInteractionChanged`. With an overlap sensor, call `RefreshInteraction` when the queue changes.
2. If `bHasInteraction` is true, show `Candidate.TitleText` and `Candidate.PromptText`.
3. Put the prompt at `Candidate.PromptWorldLocation`. If it is zero, use the target actor location.

### Show why an action is not available

Set `bEnabled` to false and put the reason in `PromptText`. The prompt shows, and the input runs nothing.

### Aim at one part of an actor

With the view trace, call `UNsInteractionComponent::GetFocusComponentOf(InActor)` in `GetInteractionCandidate`. Return false when the aimed part has no action. Set `TargetComponent` to that part.

### Hold to interact

Set `Input` to `Hold` and `HoldSeconds` to the hold time. The action runs when the hold ends. A short press runs `TapAction`, when it is bound.

### Run an action on the server

Set `Authority` to `Authority`. On a client, the component fires `OnAuthorityRequest` with the candidate. Send `TargetActor` and `TargetComponent` in a server RPC. Check the request on the server, then call `InteractAsAuthority`. On the server, the action runs at once.

### Offer two actions on one actor

Return the action that fits the current state from `GetInteractionCandidate`. Use `Priority` to win over nearby actors when the intent is not clear.

### Use a custom sensor

Call `SyncInteractionQueue` with the list of nearby actors. The component adds new ones and removes missing ones.

### React when the player comes near

Override `OnEnter` and `OnExit` on the interactable. They run when the actor enters and leaves the queue.

## Rules

- Set `PromptText` on every candidate. A candidate with empty text is discarded.
- Return false from `GetInteractionCandidate` when the actor has nothing to show. Return a disabled candidate when it must show a reason.
- Return true from the action only when the interaction ran. False logs an error.
- Do not add the owner actor to its own queue. The component skips it.

## Settings

On `FNsInteractionCandidate`:

| Field | Default | Effect |
|---|---|---|
| `TitleText` | empty | A short name of the target, above the prompt. |
| `TargetComponent` | null | The part that the candidate is for. |
| `bEnabled` | true | False shows the prompt and runs nothing. |
| `Input` | Press | `Press` runs at once. `Hold` waits for `HoldSeconds`. |
| `HoldSeconds` | 0.5 | The hold time of a hold candidate. |
| `Authority` | Local | `Authority` asks the server through `OnAuthorityRequest` on a client. |
| `TapAction` | unbound | Runs on a short press of a hold candidate. |

On `NsInteractionComponent`:

| Setting | Default | Effect |
|---|---|---|
| `InteractionRadiusCm` | 120 | The maximum distance for prompts and interaction. 0 means no limit. |
| `bUseViewTrace` | false | Traces from the view source every frame on the controlling machine. |
| `ViewTraceDistanceCm` | 300 | The length of the view trace. |
| `ViewTraceChannel` | Visibility | The channel of the view trace. |
| `CloseIntentDistanceCm` | 35 | A candidate this close wins by distance and facing only. Priority is ignored. |
| `PriorityScoreStepCm` | 24 | The distance that one priority step is worth. |
| `FacingScoreWeightCm` | 32 | The maximum penalty when the interactor faces away. |
| `bLogInteractionResolution` | false | Logs each resolve and each discarded candidate. |

Priorities: `Low`, `Normal`, `High`, `Critical`.

## Debug

- Turn on `bLogInteractionResolution` on the component.
- Log category: `LogNsInteraction`. Errors show when the resolved target left the queue, is out of range, or its action failed.
- Read `InteractionQueue` on the component in the Details panel during play.

## Limits

- No networking of its own. The game sends the server call for an `Authority` candidate.
- If a C++ class implements the interface, the component calls the C++ functions directly. A Blueprint child that overrides the same event does not run. Implement the events in one place only.
- The component ticks only with the view trace on, or during a hold.
