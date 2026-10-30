# NsInteraction Technical Documentation

## Architecture

### Modules

| Module | Type | Loading phase | Content |
|---|---|---|---|
| `NsInteraction` | Runtime | Default | The interface, the option types, the interaction component, the view sensor, and the interactable actor |

The `.uplugin` file lists no plugin dependencies.

### Folders

- `Source/NsInteraction/Public` – `NsInteraction.h`, `NsInteractionTypes.h`, `NsInteractable.h`, `NsInteractableActor.h`, `NsInteractionComponent.h`, `NsInteractionViewSensor.h`.
- `Source/NsInteraction/Private` – the `.cpp` file of each header, and `Tests/NsInteractionViewSensorTest.cpp`.

### Main Classes

| Type | Responsibility |
|---|---|
| `INsInteractable` | The interface of a usable actor. `GetInteractionOption`, `ExecuteInteraction`, `OnInteractorEnter`, `OnInteractorExit`. All are `BlueprintNativeEvent`. The defaults return false or do nothing. |
| `ANsInteractableActor` | An actor that implements `INsInteractable` with one `Option` from the Details panel. Fires `ReceiveInteract` ("On Interact") and `OnInteracted`. |
| `UNsInteractionComponent` | Holds the targets, picks the best option, runs the press and the hold, and sends server requests. |
| `UNsInteractionViewSensor` | Traces along a view source, runs the aim assist, and gives one target to the interaction component. |
| `FNsInteractionTarget` | An `Actor` and an optional `Part` (`UPrimitiveComponent`). `GetLocation()` returns the bounds origin of the part, or the actor location. |
| `FNsInteractionOption` | The target, `ActionId`, `TapActionId`, `Title`, `Prompt`, `bEnabled`, `Input`, `HoldSeconds`, `Authority`, `PromptLocation`, and `Priority`. `ShowsSame` compares the fields that the prompt shows. |
| `ENsInteractionInput` | `Press`, `Hold`. |
| `ENsInteractionAuthority` | `Local`, `Server`. |
| `FNsInteractionModule` | The module class. It has no startup code. |

### Data Flow

Targets and options:

1. A sensor calls `SetTargets`, `AddTarget`, or `RemoveTarget`. `AddTarget` and `RemoveTarget` build a new list and call `SetTargets`.
2. `SetTargets` drops invalid targets, the owner, and repeats. It calls `OnInteractorExit` on each actor that lost its last target, and `OnInteractorEnter` on each new actor. Then it calls `UpdateTickState` and `Refresh`.
3. `Refresh` skips targets out of reach (`MaxDistanceCm`). It calls `QueryOption` for each other target, scores each option with `ScoreOption`, and keeps the best. A tie keeps the first target.
4. `QueryOption` sets `OutOption.Target`, calls `GetInteractionOption`, and restores the actor. It keeps the part of the target when the interactable names no part. It sets `PromptLocation` to the target location when it is zero.
5. When `ShowsSame` reports a change, `Refresh` cancels a hold on another target and fires `OnOptionChanged`.

Input:

1. `BeginInteract` calls `Refresh`. A `Press` option runs at once. A `Hold` option stores `HoldOption` and the world time, and turns on the tick.
2. The tick runs the hold action when `GetHoldProgress()` reaches 1.
3. `EndInteract` before that cancels the hold and runs `TapActionId`, when it is set.
4. `RunAction` does nothing for a disabled option. For a `Server` option on a machine without authority, it fires `OnServerRequest`, then calls `ServerRequestAction` when `bSendServerRequests` is true. Else it calls `ExecuteInteraction`.
5. `ServerRequestAction` (Server, Reliable) calls `ExecuteOnServer`. `ExecuteOnServer` checks the reach, builds the option again with `QueryOption`, and checks `bEnabled` and that the action is `ActionId` or `TapActionId`. Then it calls `ExecuteInteraction`.

View sensor, each tick where `IsControlledHere()` is true (a pawn that this machine controls, or an owner that is not a pawn):

1. `RefreshFocus` traces from `ViewSource` along its forward vector, `TraceDistanceCm` long, on `TraceChannel`. It ignores the owner.
2. The hit part takes the focus when `QueryOption` gives an option for it.
3. Else, when `AssistAngleDeg` is above 0, `FindAssistTargets` runs a sphere overlap of radius `TraceDistanceCm`. It keeps blocking parts of actors that implement `INsInteractable`.
4. `MeasureViewAngle` finds the point of the local bounds box nearest to the view line with a ternary search of 24 steps. A sight trace drops the part when another actor is more than 10 cm in front of the point.
5. The parts sort by angle. The part of the last focus gets `AssistStickDeg` off its angle. The first part with an option takes the focus.
6. The sensor calls `SetTargets` with the focus, or `ClearTargets`.

`IsPartInView` uses the same trace and the same angle rule, with `AssistAngleDeg + AssistStickDeg + ExtraAngleDeg` as the limit. It is for a server check of a client request.

## Lifetime

- `UNsInteractionComponent` can tick, but starts with the tick off. `UpdateTickState` turns it on during a hold, or while `RefreshInterval` is above 0 and targets exist.
- `UNsInteractionComponent::EndPlay` clears the targets, so each actor gets `OnInteractorExit`.
- `UNsInteractionViewSensor` ticks every frame. `BeginPlay` finds the first `UNsInteractionComponent` and the first `UCameraComponent` on the owner, when none is set.
- `ANsInteractableActor` makes a `USceneComponent` named `Root` and does not tick. Its constructor sets `Option.ActionId` to `Use`, `Option.Prompt` to "Use", and `Option.Authority` to `Server`.

## Extension Points

- `INsInteractable` – Implement it on any actor in C++ or Blueprint:
  - `bool GetInteractionOption(AActor* Interactor, UPrimitiveComponent* Part, FNsInteractionOption& OutOption)`
  - `bool ExecuteInteraction(AActor* Interactor, const FNsInteractionOption& Option)`
  - `void OnInteractorEnter(AActor* Interactor)` and `void OnInteractorExit(AActor* Interactor)`
- `ANsInteractableActor` – `Blueprintable`. Override `GetInteractionOption` for options that change with game state. Implement `ReceiveInteract(AActor* Interactor, FName ActionId)` (`BlueprintImplementableEvent`, "On Interact"). Bind `OnInteracted` (`FNsInteractedDelegate`: `AActor* Interactor`, `FName ActionId`).
- `UNsInteractionComponent::ScoreOption(const FNsInteractionOption& Candidate) const` – `BlueprintNativeEvent`. The default returns `Priority * 100000 - Distance`.
- `OnOptionChanged` (`bool bHasOption`, `const FNsInteractionOption& Option`) and `OnServerRequest` (`const FNsInteractionOption& Option`) – `BlueprintAssignable`.
- `static bool UNsInteractionComponent::QueryOption(AActor* Interactor, const FNsInteractionTarget& Target, FNsInteractionOption& OutOption)` – For game checks and custom sensors.
- A custom sensor is any code that calls `SetTargets`.

## Dependencies

| Dependency | Kind | Why |
|---|---|---|
| `Core`, `CoreUObject`, `Engine` | Public module | Actors, components, collision queries, the camera component, and RPCs |

The `.uplugin` file lists no plugins.

## Performance

- `Refresh` calls `GetInteractionOption` one time for each target in reach.
- The view sensor ticks every frame on the controlling machine. It runs one line trace and one `GetInteractionOption` for the hit.
- With the aim assist on and no exact option, it adds one sphere overlap, then one sight trace and 48 box distance checks for each interactable part near the view. It calls `GetInteractionOption` for each part in the cone until one answers.
- The view sensor calls `SetTargets` each frame, which calls `Refresh`. A frame with a focus calls `GetInteractionOption` one more time.
- `OnOptionChanged` fires only when the shown prompt changes.

## Networking

- `UNsInteractionComponent` calls `SetIsReplicatedByDefault(true)`. It has no replicated properties. It has one RPC: `ServerRequestAction(AActor* Actor, UPrimitiveComponent* Part, FName ActionId)`, Server, Reliable.
- The RPC needs an owning connection. The owner of the component must belong to the player controller of the client.
- `ExecuteOnServer` checks the reach, the option, `bEnabled`, and the action name. It does not check the aim. Call `IsPartInView` in your own RPC for an aim check. `ExtraAngleDeg` gives room for the lag of the replicated view.
- `ANsInteractableActor` sets `bReplicates` to true. Its `Option` does not replicate.
- The view sensor runs only on the machine that controls the pawn.

## Testing

| Test | What it checks |
|---|---|
| `NsInteraction.ViewSensor.AimAssist` | The exact aim, the assist cone, the extra angle, the reach, the assist off, the edge of a large part, and a wall in front |

The test uses `EditorContext` and `EngineFilter`. It makes its own game world with `UWorld::CreateWorld` and calls only `IsPartInView`.

Run it in **Session Frontend > Automation**, filter `NsInteraction`. Or start the editor with `-ExecCmds="Automation RunTests NsInteraction"`.

## Change Rules

- Do not rename the fields of `FNsInteractionOption` or the properties of the components and `ANsInteractableActor`. Blueprints and levels save them by name.
- Keep the signatures of `INsInteractable`. Blueprint implementations of the interface break when a signature changes.
- Give a new option field a default that keeps the current behavior.
- Keep `ServerRequestAction` and `ExecuteOnServer` in step. The RPC passes only the actor, the part, and the action name, and the server builds the rest.
