# NsInteraction User Guide

## Purpose

NsInteraction lets an actor, usually the player, use other actors. A sensor gives the targets. The interaction component asks each target for its option, keeps the best one, and runs it on a press or at the end of a hold.

NsInteraction has no UI, no input binding, and no replication. The game shows the prompt, binds the input, and sends server requests. NsInteraction depends on no other plugin.

## Setup

1. Enable `NsInteraction` in the `.uproject` file.
2. Add `NsInteraction` to the module dependencies of the game.
3. Add a `UNsInteractionComponent` to the actor that interacts.
4. Pick a sensor:
   - Aim: add a `UNsInteractionViewSensor` and call `SetViewSource` with the camera.
   - Nearby: add a sphere or box collision. Bind its overlap events to `HandleSensorBeginOverlap` and `HandleSensorEndOverlap`.
   - Game code: call `AddTarget`, `RemoveTarget`, or `SetTargets`.
5. Call `BeginInteract` when the input starts and `EndInteract` when it ends.

The plugin has no project settings.

## Quick Start

C++ interactable:

```cpp
UCLASS()
class AMyLever : public AActor, public INsInteractable
{
    GENERATED_BODY()

public:
    //~ Begin INsInteractable Interface
    virtual bool GetInteractionOption_Implementation(class AActor* Interactor, class UPrimitiveComponent* Part, FNsInteractionOption& OutOption) override
    {
        OutOption.Prompt = NSLOCTEXT("MyGame", "PullLever", "Pull the lever");
        return true;
    }

    virtual bool ExecuteInteraction_Implementation(class AActor* Interactor, const FNsInteractionOption& Option) override
    {
        bPulled = !bPulled;
        return true;
    }
    //~ End INsInteractable Interface

    /** True after the pull */
    bool bPulled = false;
};
```

Blueprint interactable: add the `NsInteractable` interface in Class Settings. Implement `Get Interaction Option` and `Execute Interaction`.

## Common Tasks

### Show a prompt

Bind `OnOptionChanged`. It gives `bHasOption` and the option. Show `Option.Title`, `Option.Prompt`, and `Option.PromptLocation`. Read `GetHoldProgress` each frame for a hold bar.

### Offer more than one action

Give each action a name in `ActionId`. `ExecuteInteraction` reads `Option.ActionId` and runs that action. Return a different option for each part of the actor. The `Part` argument names the part that the sensor found.

### Hold to interact

Set `Input` to `Hold` and `HoldSeconds` to the hold time. The action runs when the hold ends. Set `TapActionId` to run another action on a short press.

### Show an action that is not available

Set `bEnabled` to false and put the reason in `Prompt`. The prompt shows, and the input runs nothing.

### Run an action on the server

1. Set `Authority` to `Server` on the option.
2. Bind `OnServerRequest`. It fires on a client, with the option and the action name.
3. Send `Option.Target.Actor`, `Option.Target.Part`, and `Option.ActionId` in a server RPC of the game.
4. On the server, check the request, for example with `UNsInteractionViewSensor::IsPartInView`.
5. Call `ExecuteOnServer`. The option builds again on the server, so the server state decides.

### Make the aim forgiving

Set `AssistAngleDeg` on the view sensor, for example to 8. When the exact aim gives no option, the part nearest to the view direction in that cone takes the focus. `AssistStickDeg` keeps the focused part, so two close parts do not flicker.

### Change how the best option wins

Make a child class of `UNsInteractionComponent` and override `ScoreOption`. The highest score wins. The default score prefers a higher `Priority`, then a nearer target.

## Rules

- Return false from `GetInteractionOption` when the part has no action now.
- A part needs a collision that blocks `TraceChannel` to take the aim.
- Do not change game state in `GetInteractionOption`. The component calls it often.
- On a client, a `Server` option does nothing until the game sends the request.

## Settings

On `UNsInteractionComponent`:

| Setting | Default | Effect |
|---|---|---|
| `RefreshInterval` | 0 | Seconds between two refreshes while there are targets. 0 refreshes only when the targets change or the input starts. |
| `MaxDistanceCm` | 0 | Most distance from the owner to a target. 0 means no limit. |

On `UNsInteractionViewSensor`:

| Setting | Default | Effect |
|---|---|---|
| `TraceDistanceCm` | 300 | Length of the trace. |
| `TraceChannel` | Visibility | Channel of the trace. |
| `AssistAngleDeg` | 0 | Half-angle of the assist cone. 0 turns the aim assist off. |
| `AssistStickDeg` | 2 | Extra angle that keeps the focused part. |

On `FNsInteractionOption`:

| Field | Default | Effect |
|---|---|---|
| `Target` | set by the component | The actor and the part. |
| `ActionId` | None | Name of the action that `ExecuteInteraction` runs. |
| `TapActionId` | None | Action of a short press on a hold option. |
| `Title` | empty | Short name of the target. |
| `Prompt` | empty | Text of the action, or the reason that it is not available. |
| `bEnabled` | true | False shows the prompt and runs nothing. |
| `Input` | Press | `Press` or `Hold`. |
| `HoldSeconds` | 0.5 | Seconds of a hold. |
| `Authority` | Local | `Local` or `Server`. |
| `PromptLocation` | zero | World point of the prompt. Zero means the target location. |
| `Priority` | 0 | A higher priority wins over a nearer target. |

## Debug

- Read `GetTargets`, `HasOption`, and `GetOption` in a debugger or a Blueprint print.
- Read `GetFocus` on the view sensor.
- Run the test `NsInteraction.ViewSensor.AimAssist` in Session Frontend.

## Limits

- One option for each target. Two actions on one part need one option at a time, or two parts.
- The view sensor gives one target at most.
- The aim assist measures the bounds box of a part, not its exact shape.
