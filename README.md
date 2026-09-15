# BTTF-HV Reversed

BTTF-HV Reversed is a native 64-bit Vice City mod that brings the Hill Valley DeLorean systems into reVC. The mod is delivered as removable `.leaf` archives and keeps its custom particles, effects, vehicle assets, sounds and native code isolated from the stock game.

The current public release is **0.1**. It includes the DeLorean, its equipment variations, cabin controls, refuelling systems, time circuits, hover and flight systems, travel effects, frost and ice, lights, mirrors, gauges, audio, and the first-person camera. The complete moved-feature list and controls are in [MIAMI-MOD.md](MIAMI-MOD.md).

## Install release 0.1

Download the [BTTF-HV-Reversed-0.1 release](https://github.com/xleafiex/BTTF-HV-Reversed/releases/tag/v0.1), extract the archive directly into a Vice City folder, and start the included `reVC.exe`. The archive contains the runtime DLLs, `mods/delorean.leaf`, and the release `mods/debug.leaf`.

The release debug archive only adds the F10 first-person camera. It does not replace, disable or alter the campaign SCM. The DeLorean spawner is part of `delorean.leaf`; press `.` when on foot to spawn it or advance its donor variation. Remove either `.leaf` archive while the game is closed to disable that feature.

## Build and install from source

This repository targets the `miami` branch with CMake 3.31, MSYS2 UCRT64 MinGW, GLFW, OpenAL and mpg123. The donor installation is used at packaging time for the vehicle and sound assets.

```powershell
.\install_miami_release.ps1
```

Use `-Build` to force a rebuild, or pass `-Install` and `-Donor` for different folders. The script builds reVC, packages the DeLorean and camera archives, copies the runtime DLLs, and installs only the current public release modules. It never launches the game.

## Controls

The main controls are documented in [MIAMI-MOD.md](MIAMI-MOD.md). The important release controls are:

* `.` — spawn or cycle DeLorean variations.
* `F10` — toggle the head-mounted first-person camera.
* `C` — hover conversion on BTTF II.
* `M` — cinematic or instant time travel.
* `+` — time circuits.
* `0`–`9`, then `-` — destination keypad. (currently only supports full date and time)
* `Tab` — refuelling at the rear, or side-hook movement at the passenger side.
* `U`/`I` — wiper controls; `J`/`K` — windows.
* `Left Shift+L`, `Right Shift+L`, or `L` — indicators and hazards.
* `P` — overhead emergency light.

## Repository layout

* `leaf-modules/delorean` — native DeLorean systems and package builder.
* `leaf-modules/debug` — first-person camera plus development-only debug helpers.
* `install_miami_release.ps1` — public release installer.
* `MIAMI-MOD.md` — detailed feature and control reference.

The repository keeps the broader native port source available for continued development; the public 0.1 release deliberately installs only the DeLorean and first-person camera modules.
