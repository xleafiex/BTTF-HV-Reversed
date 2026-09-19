# BTTF-HV Reversed — release 0.1

Native reVC Miami DeLorean port for the `miami` branch.

This branch carries the native Windows MinGW work for the Miami DeLorean project. The original donor scripts and 32-bit CLEO/ASI plugins are used as references; the shipped behavior is implemented in the 64-bit reVC Leaf modules. The modules are optional archives, so removing one while the game is closed cleanly disables that part without replacing the base game files.

The current release installer installs two archives:

* `delorean.leaf` contains the donor DeLorean model, collision, textures, sounds, cabin systems, vehicle variations and native effects.
* `debug.leaf` contains the first-person head camera only.

## Controls in the release

### Debug camera and test start

* `F10` toggles the head-mounted first-person camera on foot and in the DeLorean. Mouse look remains active, the normal free camera is preserved, and the local head is hidden while looking through the view.
* `.` spawns the DeLorean or advances its donor variation when the player is not already in a car. The variation order is BTTF I plutonium, holder-only, side-hook, BTTF II Mr. Fusion, BTTF III whitewall, horse-hook, and railroad/firebox.

### Vehicle and time circuits

* `C` starts the BTTF II hover conversion. On other variants it resets the console clock to the last departure.
* `M` switches between cinematic and instant time travel.
* `[` toggles the on-screen time-machine readout. The physical dashboard continues working.
* The mapped exit-car control shuts down the engine on the ground. Hold the accelerator to restart a stopped engine; releasing it stops cranking.
* `+` (top-row `=`/`+` or numpad `+`) toggles the time circuits.
* `0`–`9` on the top row or numpad press the matching donor keypad tones.
* Type `MMDDYYYYHHMM`, then press `-` (top-row `-` or numpad `-`) to accept a destination.
* Enter `HHMM` alone to change only the destination time, or `MMDDYYYY` alone to change only its date, then press `-`. Extra digits after a complete 12-digit entry are ignored, as in the donor.
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

## Vehicle behavior batch — September 19

The donor display-toggle control is available on `[`. BTTF III whitewall and horse-hook variants receive the donor sand-traction handling flag; railroad and other variants clear it. Stock exhaust particles are suppressed when the stock exhaust component is hidden, matching the donor component selection.

Grounded exit-key shutdown and throttle-start control use the normal mapped controls. Low-power restart attempts now occur at 200 ms intervals instead of once per rendered frame, stop immediately on throttle release, and recover within six attempts. The success rule retains the native port's bounded recovery rather than copying the contradictory donor turnover branch. The ignition key holds its starting position during cranking and restores afterward; engine-start audio is owned by one transition handler.

The cinematic implosion composites after fire trails while testing against world depth, without writing the transparent image rectangle into the depth buffer. These changes have build and offline validation; their in-game appearance and interaction still need user testing.

## Source layout

* `leaf-modules/delorean` — native DeLorean module and removable package builder.
* `leaf-modules/debug` — first-person camera
* `install_miami_release.ps1` — one-command build/package/install path for the current release.
* `build_reVC_miami.bat` — full MinGW build helper.

The repository is a work-in-progress native port. The release description above records what is actually moved into the current installable packages; source-only experiments are kept separate from the release installer.
