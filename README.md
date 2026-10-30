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
NsInteraction lets an actor, usually the player, use other actors. A sensor gives the targets. The interaction component asks each target for its option, keeps the best one, and runs it on a press or at the end of a hold. A ready-made interactable actor covers the simple case with no code.

NsInteraction has no UI and no input binding. Your game shows the prompt and calls the component from its input. NsInteraction depends on no other plugin.

## 📦 Features
- **One interface**: Implement `INsInteractable` in C++ or Blueprint. Return an option, then run it.
- **Interactable actor**: Make a Blueprint child of `ANsInteractableActor`, set its option in the Details panel, and handle `On Interact`.
- **Press or hold**: Hold for one action, and tap for another.
- **More than one action**: Give each part of an actor its own option and action name.
- **Disabled options**: Show the prompt with the reason, and run nothing.
- **Aim sensor**: A trace from the camera of the owner, with an optional aim assist cone.
- **Nearby sensor**: Bind any collision overlap, or give the targets from game code.
- **Server requests**: A server option goes to the server through a built-in RPC. The server builds the option again, so the server state decides.
- **Custom scoring**: Override `ScoreOption` to change which option wins.

## ⚙️ Requirements
Unreal Engine 5.7+

## 🛠️ Installation
1. **Clone** or download this repository.
2. Copy the `NsInteraction` folder into the `Plugins` folder of your project.
3. For C++, add `"NsInteraction"` to the dependencies of your game module in `Build.cs`.
4. Generate the project files and enable the plugin.

## 🚀 Getting Started
1. Make a Blueprint class with the parent `NsInteractableActor`. Add a static mesh to it.
2. Implement the `On Interact` event, and place the actor in the level.
3. On the player pawn, add the **Ns Interaction** and **Ns Interaction View Sensor** components. The pawn needs a camera.
4. In the input of the pawn, call `Begin Interact` when the input starts and `End Interact` when it ends.
5. Bind `On Option Changed` to show the prompt.

In C++, implement the interface on any actor:

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

## 🎯 Sensors
| Sensor | Setup |
|---|---|
| Aim | Add a `UNsInteractionViewSensor`. It uses the first camera of the owner, or the component from `SetViewSource` |
| Nearby | Add a sphere or box collision. Pass its overlap events to `HandleSensorBeginOverlap` and `HandleSensorEndOverlap` |
| Game code | Call `AddTarget`, `RemoveTarget`, or `SetTargets` |

## 🔧 API
- `INsInteractable` – The interface of a usable actor: `GetInteractionOption`, `ExecuteInteraction`, `OnInteractorEnter`, and `OnInteractorExit`.
- `ANsInteractableActor` – An actor with one option from the Details panel. It fires `On Interact` and `OnInteracted`.
- `FNsInteractionOption` – The target, the action name, the texts, `bEnabled`, press or hold, the authority, and the priority.
- `UNsInteractionComponent::BeginInteract` / `EndInteract` – Start and end the input.
- `UNsInteractionComponent::OnOptionChanged` – Fires when the shown prompt changes.
- `UNsInteractionComponent::OnServerRequest` / `ExecuteOnServer` – Fires for a server option on a client, and runs a request on the server.
- `UNsInteractionComponent::ScoreOption` – Override it to change which option wins.
- `UNsInteractionViewSensor::IsPartInView` – Check a client aim on the server.

## 🚧 Limits
- One option for each target at a time. For two actions on one part, change the option, or use two parts.
- The view sensor gives one target at most.
- The aim assist measures the bounds box of a part, not its exact shape.
- The built-in server request does not check the aim of the client.

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
