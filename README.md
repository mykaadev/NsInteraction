<!-- GH_ONLY_START -->
<h1 align="center">
  <br>
  <a href="https://github.com/mykaadev/NsInteraction"><img src="https://raw.githubusercontent.com/mykaadev/NsInteraction/refs/heads/main/Resources/Banner.png" alt="NsInteraction" width="400"></a>
</h1>

<h4 align="center">Press and hold interactions with aim, nearby targets, and server requests for Unreal Engine.</h4>

<div align="center">
    <a href="https://github.com/mykaadev/NsInteraction/commits/main"><img src="https://img.shields.io/github/last-commit/mykaadev/NsInteraction?style=plastic&logo=github&logoColor=white" alt="GitHub Last Commit"></a>
    <a href="https://github.com/mykaadev/NsInteraction/issues"><img src="https://img.shields.io/github/issues-raw/mykaadev/NsInteraction?style=plastic&logo=github&logoColor=white" alt="GitHub Issues"></a>
    <a href="https://github.com/mykaadev/NsInteraction/pulls"><img src="https://img.shields.io/github/issues-pr-raw/mykaadev/NsInteraction?style=plastic&logo=github&logoColor=white" alt="GitHub Pull Requests"></a>
    <a href="https://github.com/mykaadev/NsInteraction"><img src="https://img.shields.io/github/stars/mykaadev/NsInteraction?style=plastic&logo=github" alt="GitHub Stars"></a>
    <a href="https://twitter.com/mykaadev/"><img src="https://img.shields.io/twitter/follow/mykaadev?style=plastic&logo=x" alt="Twitter Follow"></a>

<p style="display:none;">
  <a href="#-summary">👀 Summary</a> •
  <a href="#-features">📦 Features</a> •
  <a href="#-requirements">⚙️ Requirements</a> •
  <a href="#-installation">🛠️ Installation</a> •
  <a href="#-getting-started">🚀 Getting Started</a> •
  <a href="#-sensors">🎯 Sensors</a> •
  <a href="#-api">🔧 API</a> •
  <a href="#-limits">🚧 Limits</a> •
  <a href="#-documentation">📚 Documentation</a> •
  <a href="#-credits">❤️ Credits</a> •
  <a href="#-support">📞 Support</a> •
  <a href="#-license">📃 License</a>
</p>
<a href="https://buymeacoffee.com/mykaadev"><img src="https://www.svgrepo.com/show/476855/coffee-to-go.svg" alt="Coffee" width="50px"></a>
<p><b>Buy me a coffee!</b></p>
</div>
<!-- GH_ONLY_END -->

## 👀 Summary
NsInteraction lets an actor, usually the player, use other actors. A sensor gives the targets. The interaction component asks each target for its option, keeps the best one, and runs it on a press or at the end of a hold.

NsInteraction has no UI, no input binding, and no replication. The game shows the prompt, binds the input, and sends the server requests. NsInteraction depends on no other plugin.

## 📦 Features
- **One interface**: Implement `INsInteractable` in C++ or Blueprint. Return an option, then run it.
- **Press or hold**: Hold for one action, tap for another.
- **More than one action**: Give each part of an actor its own option and action name.
- **Disabled options**: Show the prompt with the reason, and run nothing.
- **Aim sensor**: A trace from the camera, with an optional aim assist cone.
- **Nearby sensor**: Bind any collision overlap, or give the targets from game code.
- **Server requests**: A server option builds again on the server, so the server state decides.
- **Custom scoring**: Override `ScoreOption` to change which option wins.

## ⚙️ Requirements
Unreal Engine 5.7+

## 🛠️ Installation
1. **Clone** or download this repository.
2. Copy the `NsInteraction` folder into the `Plugins` folder of your project.
3. Add `"NsInteraction"` to the module dependencies in `Build.cs`.
4. Generate the project files and enable the plugin.
5. Add a `UNsInteractionComponent` and a sensor to the actor that interacts.

The plugin has no project settings.

## 🚀 Getting Started
```cpp
#include "NsInteractable.h"

UCLASS()
class AMyLever : public AActor, public INsInteractable
{
    GENERATED_BODY()

public:
    //~ Begin INsInteractable Interface
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
    //~ End INsInteractable Interface

    /** True after the pull */
    bool bPulled = false;
};
```

On the player, call `BeginInteract` when the input starts and `EndInteract` when it ends. Bind `OnOptionChanged` to show the prompt, and read `GetHoldProgress` for a hold bar.

In Blueprint, add the `NsInteractable` interface in Class Settings, then implement `Get Interaction Option` and `Execute Interaction`.

## 🎯 Sensors
| Sensor | Setup |
|---|---|
| Aim | Add a `UNsInteractionViewSensor` and call `SetViewSource` with the camera |
| Nearby | Add a sphere or box collision. Bind its overlaps to `HandleSensorBeginOverlap` and `HandleSensorEndOverlap` |
| Game code | Call `AddTarget`, `RemoveTarget`, or `SetTargets` |

## 🔧 API
### Types
- `INsInteractable` – The interface of an actor that can be used: `GetInteractionOption` and `ExecuteInteraction`.
- `FNsInteractionOption` – The target, the action name, the prompt, `bEnabled`, press or hold, the authority, and the priority.

### Main Functions
- `UNsInteractionComponent::BeginInteract` / `EndInteract` – Start and end the input.
- `UNsInteractionComponent::OnOptionChanged` – Fires when the best option changes.
- `UNsInteractionComponent::OnServerRequest` / `ExecuteOnServer` – Send a server option through a server RPC of the game, then run it on the server.
- `UNsInteractionComponent::ScoreOption` – Override to change which option wins.
- `UNsInteractionViewSensor::IsPartInView` – Check a client request on the server.

### Settings
- `RefreshInterval`, `MaxDistanceCm` – On the interaction component.
- `TraceDistanceCm`, `TraceChannel`, `AssistAngleDeg`, `AssistStickDeg` – On the view sensor.

## 🚧 Limits
- One option for each target. Two actions on one part need one option at a time, or two parts.
- The view sensor gives one target at most.
- The aim assist measures the bounds box of a part, not its exact shape.

## 📚 Documentation
- [User Guide](https://mykaadev.github.io/plugins/nsinteraction/user-guide/) – Setup, common tasks, settings, and debug tools.
- [Technical Documentation](https://mykaadev.github.io/plugins/nsinteraction/tech-documentation/) – Architecture, lifetime, extension points, and tests.

<!-- GH_ONLY_START -->
## ❤️ Credits
<a href="https://github.com/mykaadev/NsInteraction/graphs/contributors"><img src="https://contrib.rocks/image?repo=mykaadev/NsInteraction"/></a>

## 📞 Support
Reach out via the **[profile page](https://github.com/mykaadev)**.

## 📃 License
[![License](https://img.shields.io/badge/license-MIT-green)](https://www.tldrlegal.com/license/mit-license)
<!-- GH_ONLY_END -->
