# NsInteraction Technical Documentation

## Architecture

One runtime module, `NsInteraction`. Four public headers:

| Header | Content |
|---|---|
| `NsInteractionTypes.h` | `FNsInteractionTarget`, `FNsInteractionOption`, `ENsInteractionInput`, `ENsInteractionAuthority` |
| `NsInteractable.h` | `INsInteractable`: `GetInteractionOption`, `ExecuteInteraction`, `OnInteractorEnter`, `OnInteractorExit` |
| `NsInteractionComponent.h` | `UNsInteractionComponent`: the targets, the best option, the press and the hold, the server request |
| `NsInteractionViewSensor.h` | `UNsInteractionViewSensor`: the view trace and the aim assist |

Data flow:

1. A sensor calls `SetTargets`, `AddTarget`, or `RemoveTarget`.
2. `SetTargets` sends `OnInteractorEnter` to a new actor and `OnInteractorExit` to an actor that left. Then it calls `Refresh`.
3. `Refresh` calls `QueryOption` for each target in reach, scores each option with `ScoreOption`, and keeps the best one. A tie keeps the first target.
4. `OnOptionChanged` fires when `ShowsSame` says that the shown prompt changed.
5. `BeginInteract` refreshes, then runs a press option or starts a hold. `EndInteract` before the end of a hold runs `TapActionId`.
6. To run an action: a disabled option does nothing. A `Server` option on a client fires `OnServerRequest`. Any other option calls `ExecuteInteraction`.
7. `ExecuteOnServer` builds the option again from the requested target and runs the requested action, when the option names that action.

View sensor flow:

1. Each tick on the machine that controls the owner, the sensor traces along the view source.
2. The hit part takes the focus when its actor gives an option.
3. Else, with `AssistAngleDeg` above 0, a sphere overlap collects the interactable parts that block the channel. `MeasureViewAngle` finds the point of the local bounds box nearest to the view line with a ternary search. A sight trace drops a part behind another actor.
4. The parts sort by angle. The focused part gets `AssistStickDeg` of bonus. The first part with an option takes the focus.
5. The sensor calls `SetTargets` with the focus, or `ClearTargets`.

## Lifetime

- The component ticks only during a hold, or while `RefreshInterval` is above 0 and targets exist.
- `EndPlay` clears the targets, so each actor gets its exit.
- The view sensor ticks every frame. `BeginPlay` finds the interaction component on the owner when none is set.

## Extension Points

- `INsInteractable`: the game implements it on any actor, in C++ or in Blueprint.
- `ScoreOption`: override it in a child class of the component.
- `OnOptionChanged` and `OnServerRequest`: the adapters of the game bind them for the UI and for the network.
- A custom sensor: any code can call `SetTargets`.
- `QueryOption`: a static call that game code can use for its own checks.

## Dependencies

`Core`, `CoreUObject`, and `Engine`. No other plugin.

## Performance

- `Refresh` calls `GetInteractionOption` once for each target.
- The view sensor calls `GetInteractionOption` once for the exact hit. With the aim assist and no exact option, it adds one sphere overlap, one sight trace for each part near the view, and one `GetInteractionOption` for each part in the cone until one answers.
- The view sensor calls `SetTargets`, which refreshes, so a frame with a focus calls `GetInteractionOption` one more time.

## Networking

The plugin does not replicate. `OnServerRequest` gives the game what it must send to the server. `ExecuteOnServer` runs it there. `IsPartInView` lets the server check a client aim, with `ExtraAngleDeg` for the lag of the replicated view.

## Testing

- `NsInteraction.ViewSensor.AimAssist`: the exact aim, the assist cone, the extra angle, the reach, a large part, and a wall in front.
- `AActor::ProcessEvent` runs interface events only in a world with initialized actors. A test world calls `InitializeActorsForPlay` before it asks for options.

## Change Rules

- Keep `INsInteractable` and `FNsInteractionOption` stable. Every interactable of every game uses them.
- Add a new option field with a default that keeps the current behavior.
- Keep the plugin free of other plugins. A link to another plugin goes into the game, as an adapter.
