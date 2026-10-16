# NsInteraction Technical Documentation

## Architecture

One runtime module, `NsInteraction`.

| Type | Role |
|---|---|
| `UNsInteractionComponent` | Actor component on the interactor. It owns the queue, the optional view trace, the focus, and the hold. It resolves the best candidate and runs it. |
| `UNsInteractionPromptWidget` | Abstract prompt widget. Optional parts: `TitleText`, `ActionText`, `KeyBadge`, `KeyText`, `KeyIcon`, `HoldProgressBar`. |
| `INsInteractable` | Interface on interactable actors: `OnEnter`, `OnExit`, `OnInteract`, `GetInteractionWorldLocation`, `GetInteractionCandidate`, `ExecuteInteractionCandidate`. |
| `FNsInteractionCandidate` | One option: `TargetActor`, `TargetComponent`, `TitleText`, `PromptText`, `bEnabled`, `Input`, `HoldSeconds`, `Authority`, `PromptWorldLocation`, `Priority`, `Action`, `TapAction`. |
| `FNsInteractionAction` | A native `TFunction<bool(AActor*)>` or a Blueprint dynamic delegate. Native wins when both are set. |
| `FNsResolvedInteraction` | `bHasInteraction`, the winning `Candidate`, and all valid `Candidates` sorted best first. |
| `ENsInteractionPriority` | `Low` (0), `Normal` (1), `High` (2), `Critical` (3). |
| `ENsInteractionInput` | `Press` or `Hold`. |
| `ENsInteractionAuthority` | `Local` or `Authority`. |
| `UNsInteractionCandidateLibrary` | Blueprint helpers `SetCandidateAction` and `SetCandidateTapAction`. |

### Resolve flow

1. For each valid queued actor, call `GetInteractionCandidate`. If `TargetActor` is null, set it to the queued actor.
2. Discard candidates with an invalid target or empty `PromptText`.
3. Distance is measured from the owner to `PromptWorldLocation`, or to the root component location if that is zero.
4. Discard candidates beyond `InteractionRadiusCm`, when the radius is above 0.
5. Score each candidate. A lower score is better.
6. Sort by score, then distance, then queue order.

### Score formula

```text
Facing = (1 - dot(OwnerForward2D, ToCandidate2D)) * FacingScoreWeightCm
If Distance <= CloseIntentDistanceCm:
    Score = Distance + Facing
Else:
    Score = Distance + Facing - (PriorityValue - 1) * PriorityScoreStepCm
If any candidate is inside CloseIntentDistanceCm:
    add 100000 to every candidate outside it
```

### View trace flow

1. The tick traces from the view source along its forward vector, when the owner is a locally controlled pawn or not a pawn.
2. The hit actor is the only queued actor. The hit becomes the focus.
3. `RefreshInteraction` resolves and stores the result. `OnInteractionChanged` fires when the target, the part, the texts, the input, or the enable state change.
4. A change of target or part cancels a hold.

### Input flow

1. `BeginInteract` takes the stored result. Without the view trace, it resolves first.
2. A hold candidate starts the hold. The tick runs the candidate when `GetHoldProgress` reaches 1.
3. `EndInteract` before the end cancels the hold and runs `TapAction`.
4. To run a candidate: a disabled one does nothing. An `Authority` candidate on a client fires `OnAuthorityRequest`. Any other runs through the interact flow below.
5. `InteractAsAuthority` sets the focus to the requested part, resolves, and runs an enabled candidate for the requested target.

### Interact flow

`Interact` calls `InteractResolved`, which resolves the queue and calls `InteractWithResolvedInteraction`:

1. Stop if there is no candidate, or if the candidate is disabled.
2. Log an error and stop if the target left the queue, is out of range, or is not interactable.
3. If the candidate has an action, run it. Log an error if it returns false.
4. If not, call `ExecuteInteractionCandidate` on the target.
5. If that returns false, call `OnInteract` on the target.

## Lifetime

- The component ticks only while `bUseViewTrace` is on, or during a hold.
- The queue changes only through the overlap handlers, `AddToInteractionQueue`, `RemoveFromInteractionQueue`, `ClearInteractionQueue`, and `SyncInteractionQueue`.
- `SyncInteractionQueue` removes destroyed actors.
- `ResolveInteractionOwner` falls back to the typed outer when there is no owner. Tests use this.

## Extension Points

| Point | Use |
|---|---|
| `INsInteractable` events | `BlueprintNativeEvent`. Implement in C++ or Blueprint. |
| `FNsInteractionAction` | Bind per candidate, so one actor can offer different actions. |
| `ResolveInteractionCandidates` | Public. A game can resolve its own candidate list with the same rules. |
| Component settings | Tune the radius and the score weights per interactor. |

## Dependencies

`Core`, `CoreUObject`, `Engine`, `UMG`, `Slate`, `SlateCore`. No other studio plugin.

## Performance

- Each resolve calls `GetInteractionCandidate` on every queued actor and sorts the result.
- Keep the queue small with a tight sensor. Resolve on queue changes or on a timer, not every frame.
- The view trace queues one actor at most, so its resolve every frame calls one `GetInteractionCandidate`.

## Networking

None of its own. The queue and the resolve are local to each machine. An `Authority` candidate on a client fires `OnAuthorityRequest`. The game sends a server RPC, checks it, and calls `InteractAsAuthority` on the server.

## Testing

NsInteraction has no automated tests. Manual test path:

1. Put two interactables at different distances. Make sure that the closer one wins.
2. Set the far one to `Critical`. Make sure that it wins when neither is inside `CloseIntentDistanceCm`.
3. Walk inside `CloseIntentDistanceCm` of the other one. Make sure that it wins again.
4. Turn on `bLogInteractionResolution` and read the discard reasons.

## Change Rules

- Keep the interface signatures stable. Blueprints in projects implement them.
- The native fast path (`Cast<INsInteractable>` then `_Implementation`) skips Blueprint overrides on C++ classes. A fix changes behavior for current projects. Use Plugin Feature Fit before a change.
- New candidate fields keep a default that matches the old behavior: enabled, press, local.
