# BTTF-HV Reversed — release 0.1

Native reVC Miami DeLorean port for the `miami` branch.

This branch carries the native Windows MinGW work for the Miami DeLorean project. The original donor scripts and 32-bit CLEO/ASI plugins are used as references; the shipped behavior is implemented in the 64-bit reVC Leaf modules. The modules are optional archives, so removing one while the game is closed cleanly disables that part without replacing the base game files.

The current release installer installs two archives:

* `delorean.leaf` contains the donor DeLorean model, collision, textures, sounds, cabin systems, vehicle variations and native effects.
* `debug.leaf` contains a small freeroam SCM for a clean test start and the first-person head camera. The release debug archive intentionally does not include the development fire-preview or body-opacity controls.

Graphics and ragdoll work remains in the source tree for development, but is not installed by the release installer yet. The normal reVC anti-aliasing path remains in charge of anti-aliasing.

## Install

1. Close reVC.
2. Check out the `miami` branch and make sure the donor folder exists at `E:\My Documents\crap\modding\BTTF HV\current wip\Grand Theft Auto Vice City (HV)`, or pass your donor path to the script.
3. From the repository root, run:

```powershell
.\install_miami_release.ps1
```

The script builds the matching `reVC.exe` when needed, creates the two Leaf archives, and copies them to `mods` in the game folder. To force a rebuild, use `.\install_miami_release.ps1 -Build`. Custom paths are supported with `-Install` and `-Donor`.

To remove the port, close the game and delete `mods\delorean.leaf` and `mods\debug.leaf`. The base game files are not modified by the Leaf archives.

## Controls in the release

### Debug camera and test start

* `F10` toggles the head-mounted first-person camera on foot and in the DeLorean. Mouse look remains active, the normal free camera is preserved, and the local head is hidden while looking through the view.
* The release `debug.leaf` supplies a stripped freeroam SCM so a new game starts directly in a test scene instead of starting the campaign. Removing `debug.leaf` restores the campaign SCM.
* `.` spawns the DeLorean or advances its donor variation when the player is not already in a car. The variation order is BTTF I plutonium, holder-only, side-hook, BTTF II Mr. Fusion, BTTF III whitewall, horse-hook, and railroad/firebox.

### Vehicle and time circuits

* `Enter` uses the normal vehicle entry and door animation.
* `C` starts the BTTF II hover conversion. On other variants it resets the console clock to the last departure.
* `M` switches between cinematic and instant time travel.
* `+` (top-row `=`/`+` or numpad `+`) toggles the time circuits.
* `0`–`9` on the top row or numpad press the matching donor keypad tones.
* Type `MMDDYYYYHHMM`, then press `-` (top-row `-` or numpad `-`) to accept a destination.
* Hold the accelerator at 88 MPH with circuits and fuel available to trigger time travel.
* `Tab` at the rear starts the Mr. Fusion/plutonium refuel sequence. On the side-hook variation, `Tab` at the passenger side moves the hook between its side and rear positions; `Tab` at the rear refuels when the hook is not deployed.
* The period variation cycle is the complete selector for BTTF I, II and III equipment.

### Cabin systems

* `U` cycles wipers through normal, fast and off; intermittent mode parks with `U`.
* `I` operates the reverse/single-sweep wiper stalk.
* `J` and `K` lower or raise the left and right windows.
* `Left Shift` + `L` toggles the left indicator; `Right Shift` + `L` toggles the right indicator; `L` by itself toggles hazards. Press the same combination again to turn it off.
* `P` toggles the overhead emergency light, its handle and donor sound.
* Horn input moves the mapped turn-signal stalk.
* The steering wheel, pedals, gear lever, compass, console clock, gauges, warning lamps, wipers, windows, frost, lights and donor cabin animations follow vehicle state.

### Vehicle features included so far

The native module loads the donor DeLorean DFF/TXD/COL and audio, preserves the front gullwing door frames and entry animation, supports BTTF I/II/III equipment variations, side and rear hook states, Mr. Fusion and plutonium refuelling, time-circuit displays, console clock, digital and analog speed indication, reactor power/fuel behavior, wipers, windows, indicators, hazards, running/reverse/brake lighting, emergency cabin lighting, headlights, wheel conversion, hover/landing, underbody chasers, wheel plasma, wormhole and implosion effects, cinematic departure/re-entry, fire trails, blue travel sparks, frost, detached OUTATIME plate behavior, mirror views, and donor-attached engine/hover audio.

The native module keeps its custom particle presets and textures inside `delorean.leaf` under `particles_additional.cfg` and `particles_additional.txd`, so stock Vice City particle definitions are not overwritten. The DeLorean package is assembled from the donor folder at install time; donor model and sound assets are not copied into the Git source tree.

## Source layout

* `leaf-modules/delorean` — native DeLorean module and removable package builder.
* `leaf-modules/debug` — first-person camera, release SCM wrapper, and development-only debug helpers.
* `leaf-modules/graphics` — source-only graphics module.
* `leaf-modules/ragdoll` — source-only Bullet prototype.
* `install_miami_release.ps1` — one-command build/package/install path for the current release.
* `build_reVC_miami.bat` — full MinGW build helper.

The repository is a work-in-progress native port. The release description above records what is actually moved into the current installable packages; source-only experiments are kept separate from the release installer.
