# NsInteraction User Guide

## Purpose

NsInteraction lets an actor, usually the player, interact with nearby actors. It keeps a queue of nearby interactables, picks the best one by distance, facing, and priority, and runs its action.

NsInteraction does not detect nearby actors by itself, show a prompt widget, or replicate. The game supplies a sensor, the prompt UI, and any server call.

## Setup

1. Add `NsInteraction` to the `Plugins` list of the `.uproject` file.
2. For C++, add `"NsInteraction"` to the module dependencies in `Build.cs`.

## Quick Start

### 1. The interactor

1. Add a `NsInteractionComponent` to the player pawn.
2. Add a sphere or box collision component as a sensor.
3. Bind the sensor overlap events to `HandleInteractionSensorBeginOverlap` and `HandleInteractionSensorEndOverlap`.
4. Call `Interact` from the interact input action.

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

## Common Tasks

### Show a prompt

1. Call `ResolveCurrentInteraction` when the queue changes, or on a short timer.
2. If `bHasInteraction` is true, show `Candidate.PromptText`.
3. Put the prompt at `Candidate.PromptWorldLocation`. If it is zero, use the target actor location.

### Offer two actions on one actor

Return the action that fits the current state from `GetInteractionCandidate`. Use `Priority` to win over nearby actors when the intent is not clear.

### Use a custom sensor

Call `SyncInteractionQueue` with the list of nearby actors. The component adds new ones and removes missing ones.

### React when the player comes near

Override `OnEnter` and `OnExit` on the interactable. They run when the actor enters and leaves the queue.

## Rules

- Set `PromptText` on every candidate. A candidate with empty text is discarded.
- Return false from `GetInteractionCandidate` when the actor cannot be used now.
- Return true from the action only when the interaction ran. False logs an error.
- Do not add the owner actor to its own queue. The component skips it.

## Settings

On `NsInteractionComponent`:

| Setting | Default | Effect |
|---|---|---|
| `InteractionRadiusCm` | 120 | The maximum distance for prompts and interaction. 0 means no limit. |
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

- No networking. In multiplayer, call a server RPC from the game and call `Interact` on the server.
- If a C++ class implements the interface, the component calls the C++ functions directly. A Blueprint child that overrides the same event does not run. Implement the events in one place only.
- The summary log of `ResolveCurrentInteraction` goes to `LogTemp`, not `LogNsInteraction`.
- The component does not tick. Resolve on events or on a timer.
