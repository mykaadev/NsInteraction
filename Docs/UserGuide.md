# NsInteraction User Guide

## Purpose

NsInteraction lets an actor, usually the player, use other actors: doors, levers, pickups, and buttons. A sensor gives the targets. The interaction component asks each target for its option, keeps the best one, and runs it on a press or at the end of a hold.

NsInteraction has no UI and no input binding. Your game shows the prompt and calls the component from its input. NsInteraction depends on no other plugin.

## Setup

1. Copy the `NsInteraction` folder into the `Plugins` folder of your project.
2. Open the project and enable NsInteraction in **Edit > Plugins**.
3. For C++, add `"NsInteraction"` to the dependencies of your game module in `Build.cs`.
4. Restart the editor.

The plugin has no project settings.

## Quick Start

1. Make a Blueprint class with the parent `NsInteractableActor`. Name it `BP_Button`.
2. Add a **Static Mesh** component to `BP_Button` and set a mesh. Its collision must block the **Visibility** channel.
3. In the Event Graph, implement `On Interact` and add a `Print String` node.
4. Place `BP_Button` in the level.
5. Open your player pawn. Make sure that it has a **Camera** component.
6. Add the **Ns Interaction** component and the **Ns Interaction View Sensor** component to the pawn.
7. In the input of the pawn, call `Begin Interact` on the **Ns Interaction** component when the input starts.
8. Call `End Interact` when the input ends.
9. Press Play, look at the button from less than 3 m, and press the input. The text prints.

In C++, implement `INsInteractable` on any actor:

```cpp
// MyLever.h
#include "GameFramework/Actor.h"
#include "NsInteractable.h"
#include "MyLever.generated.h"

UCLASS()
class AMyLever : public AActor, public INsInteractable
{
    GENERATED_BODY()

public:
    virtual bool GetInteractionOption_Implementation(AActor* Interactor, UPrimitiveComponent* Part, FNsInteractionOption& OutOption) override
    {
        OutOption.Prompt = NSLOCTEXT("MyGame", "PullLever", "Pull the lever");
        return true;
    }

    virtual bool ExecuteInteraction_Implementation(AActor* Interactor, const FNsInteractionOption& Option) override
    {
        bPulled = !bPulled;
        return true;
    }

    bool bPulled = false;
};
```

## Common Tasks

### Make an interactable with no code

1. Make a Blueprint class with the parent `NsInteractableActor`.
2. Add the meshes. Each mesh that takes the aim must block the trace channel of the view sensor.
3. In the Details panel, open **Interaction > Option**.
4. Set **Prompt**, **Title**, **Input**, and the other fields.
5. Implement `On Interact`, or bind `On Interacted`.

The default option has **Action Id** `Use`, **Prompt** "Use", and **Authority** `Server`. To turn the option on or off in play, call `Set Interaction Enabled`.

### Make any Blueprint interactable

1. Open the Blueprint and select **Class Settings**.
2. Under **Interfaces**, add `NsInteractable`.
3. Implement `Get Interaction Option`. Set the fields of **Out Option** and return true.
4. Implement `Execute Interaction`. Read **Action Id** from the option and run the action.
5. Optional: implement `On Interactor Enter` and `On Interactor Exit` to react when an interactor starts or stops to consider the actor.

Return false from `Get Interaction Option` when the part has no action now. Do not change game state in it, because the component calls it often.

### Aim at targets

1. Add the **Ns Interaction View Sensor** component to the pawn.
2. Make sure that the pawn has a **Camera** component, or call `Set View Source` with another scene component.
3. Set **Trace Distance Cm** to the reach of the aim.

The sensor uses the first camera of the owner and the first **Ns Interaction** component of the owner. It traces each frame on the machine that controls the pawn.

### Use nearby targets

1. Add a **Sphere Collision** or **Box Collision** component to the pawn.
2. On its `On Component Begin Overlap` event, call `Handle Sensor Begin Overlap` on the **Ns Interaction** component. Pass all pins.
3. On its `On Component End Overlap` event, call `Handle Sensor End Overlap` the same way.

In C++, bind the overlap delegates to `UNsInteractionComponent::HandleSensorBeginOverlap` and `HandleSensorEndOverlap` with `AddDynamic`.

### Give targets from game code

1. Call `Add Target` with an actor and an optional part.
2. Call `Remove Target` with the actor to remove all of its targets.
3. Call `Set Targets` to replace the full list. The first target wins a tie.

### Show a prompt

1. Bind `On Option Changed` on the **Ns Interaction** component.
2. When **Has Option** is true, show **Title** and **Prompt** of the option at **Prompt Location**.
3. When **Has Option** is false, hide the prompt.
4. For a hold, read `Get Hold Progress` each frame. It goes from 0 to 1.

### Hold to interact

1. Set **Input** of the option to `Hold`.
2. Set **Hold Seconds** to the hold time.
3. Optional: set **Tap Action Id** to run another action on a short press.

The action runs when the hold time ends. A release before the end runs the tap action, when it is set. The hold stops when the target changes.

### Offer more than one action

1. Give each part of the actor its own option in `Get Interaction Option`. The **Part** input names the part that the sensor found.
2. Give each option its own **Action Id**.
3. In `Execute Interaction`, run the action that **Action Id** names.

### Show an action that is not available

1. Set **Enabled** of the option to false.
2. Put the reason in **Prompt**.

The prompt shows, and the input runs nothing.

### Run an action on the server

1. Set **Authority** of the option to `Server`.
2. Keep **Send Server Requests** on the **Ns Interaction** component on.
3. Set **Max Distance Cm** on the **Ns Interaction** component, so the server checks the reach.

On a client, the component fires `On Server Request` and sends the request to the server. The server builds the option again and runs it when the option is enabled and names the same action. The pawn must belong to the player controller of the client.

To send the request yourself, clear **Send Server Requests**. Bind `On Server Request`, send the target and the action in your own server RPC, and call `Execute On Server` there. You can check the aim first with `Is Part In View` on the view sensor.

### Make the aim forgiving

1. Set **Assist Angle Deg** on the view sensor, for example to 8.
2. Keep **Assist Stick Deg** at 2 or more, so two close parts do not flicker.

When the exact aim gives no option, the part nearest to the view direction inside the cone takes the focus.

### Change which option wins

1. Make a child class of the **Ns Interaction** component.
2. Override `Score Option`. The highest score wins.

The default score prefers a higher **Priority**, then a nearer target.

## Settings

### Ns Interaction component

| Setting | Default | Effect |
|---|---|---|
| **Refresh Interval** | 0 s | Seconds between two refreshes while there are targets. 0 refreshes only when the targets change or the input starts. |
| **Send Server Requests** | true | Send a server option to the server with the built-in RPC. When false, only `On Server Request` fires. |
| **Max Distance Cm** | 0 | Most distance from the owner to a target. 0 means no limit. |

### Ns Interaction View Sensor component

| Setting | Default | Effect |
|---|---|---|
| **Trace Distance Cm** | 300 | Length of the trace and radius of the assist search. |
| **Trace Channel** | Visibility | Channel of the trace. A part must block it to take the aim. |
| **Assist Angle Deg** | 0 | Half-angle of the assist cone, up to 45. 0 turns the aim assist off. |
| **Assist Stick Deg** | 2 | Extra angle that keeps the focused part. |

### Interaction option

| Field | Default | Effect |
|---|---|---|
| **Target** | Set by the component | The actor and the part. |
| **Action Id** | None | Name of the action that `Execute Interaction` runs. |
| **Tap Action Id** | None | Action of a short press on a hold option. None does nothing. |
| **Title** | Empty | Short name of the target. |
| **Prompt** | Empty | Text of the action, or the reason that it is not available. |
| **Enabled** | true | False shows the prompt and runs nothing. |
| **Input** | `Press` | `Press` or `Hold`. |
| **Hold Seconds** | 0.5 | Seconds of a hold. |
| **Authority** | `Local` | `Local` runs on the machine of the interactor. `Server` runs on the server. |
| **Prompt Location** | Zero | World point of the prompt. Zero means the center of the part, or the actor location. |
| **Priority** | 0 | A higher priority wins over a nearer target. |

On **Ns Interactable Actor**, the option defaults are **Action Id** `Use`, **Prompt** "Use", and **Authority** `Server`.

## Debug

The plugin has no console commands, console variables, log category, or debug draw.

To see the state in play, print these values:

- `Get Targets`, `Has Option`, and `Get Option` on the **Ns Interaction** component.
- `Get Focus` on the **Ns Interaction View Sensor** component.

## Troubleshooting

| Problem | Cause | Fix |
|---|---|---|
| The view sensor finds nothing. | The owner has no **Camera** component and no view source is set. | Add a camera, or call `Set View Source`. |
| The aim passes through a mesh. | The mesh does not block **Trace Channel**. | Set the collision of the mesh to block the channel. |
| The prompt never shows. | `Get Interaction Option` returns false, or the target is farther than **Max Distance Cm**. | Return true, or raise **Max Distance Cm**. |
| `Begin Interact` returns false. | No target gives an option now. | Check the sensor and `Get Interaction Option`. |
| A server option does nothing on a client. | **Send Server Requests** is off, or the pawn does not belong to the player controller. | Turn it on, or send the request from your own RPC. |
| The server does not run a request. | The option on the server is disabled, out of reach, or has another **Action Id**. | Make the server state match the client prompt. |
| The hold stops before the end. | The best target changed during the hold. | Keep the aim on the target, or raise **Assist Stick Deg**. |
| The focus jumps between two close parts. | **Assist Stick Deg** is too small. | Raise **Assist Stick Deg**. |
| A client still shows a prompt after `Set Interaction Enabled` on the server. | The option of **Ns Interactable Actor** does not replicate. | Call `Set Interaction Enabled` on each machine, for example from a replicated variable. |

## Limits

- The option of **Ns Interactable Actor** does not replicate. `Set Interaction Enabled` changes it only on the machine that calls it.
- One option for each target at a time. For two actions on one part, change the option, or use two parts.
- The view sensor gives one target at most.
- The aim assist measures the bounds box of a part, not its exact shape.
- The built-in server request checks the reach and the option, but not the aim of the client.
