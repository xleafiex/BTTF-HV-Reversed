# reVC Miami Leaf packages

The installed build is 64-bit reVC with four optional `.leaf` archives. Remove any archive while the game is closed to disable that feature.

- `delorean.leaf` loads the donor DeLorean DFF/TXD/COL and sounds and provides native spawn, hover, circuits, refuelling, and time-travel controls.
- Front gullwing doors are kept as intact donor frames, clear the stock no-door handling flag, and start in an OK damage state so normal `Enter` entry drives the door and actuator animation.
- `graphics.leaf` provides the native GL3 water swell/lighting pass, sky/cloud pass, and weather hotkey.
- `debug.leaf` supplies a stripped freeroam SCM and toggles a head camera on F10 without changing FreeCam. The release archive omits development-only F9 fire preview and body-opacity controls. Head-camera gameplay validation remains pending.
- `ragdoll.leaf` contains Bullet DLLs and a removable whole-body player/nearby-ped prototype on F6.

The donor CLEO/ASI binaries remain 32-bit and are not loaded by this 64-bit build. Scene refraction, shore foam, shadows, matcap, advanced particles, and articulated bone ragdolls remain future work. Anti-aliasing stays with reVC.

## Cabin and refueling batch — September 10, 2026

Implemented from the supplied donor's `Delorean/Dashboard.txt`, `Include/Fuel.txt`, and `Include/Effects.txt`:

- Oil pressure, temperature and petrol needles; voltage/startup dip; occupied/unoccupied ignition positions; oil, battery, seatbelt, low-beam, fuel and lambda lamps. The petrol gauge is distinct from time-travel fuel.
- Glow initialization for donor needles, warning lights, time-display digits and authored thruster frames. Existing coil alpha/wormhole rendering has not been certified visually.
- Mr. Fusion unlatch/open/hold/one randomly selected trash clip/close/relatch sequence. Closing sound starts before the lid moves. Steam intentionally starts with the red latch per the user's preference, earlier than the donor script.
- Plutonium lid removal, canister lowering/rotation, pellet drop, canister removal and lid replacement stages, including the separate hold delays and sound cues.
- Rear interaction box and player heading alignment; movement locked through 500 ms after closure and fuel granted 500 ms later. Lock is released on cancellation, vehicle loss, death and module shutdown.
- Reactor-empty lamp is shown when unfueled. Named OpenAL voices allow different sounds and hover loops to coexist, follow the reactor position, respect effects volume, pause/resume and clean up independently. Stereo donor WAVs are mixed to mono for spatial playback. This establishes concurrent positional playback, not full donor engine/hover sound-state parity.

Animation steps use a stable 30 fps donor baseline: wait-10 loops advance one step per baseline frame, wait-50 loops two. They do not run at reVC's 50 timestep units/second. All 50 donor WAV files use the supported PCM format; assets and particle files remain byte-identical to the previous package.

Validation: MinGW build passed; standalone `donor_systems_test.cpp` covers 15–144 fps, gauge targets, reverse throttle, cue counts and completion delays. `donor_audio_test.cpp` used OpenAL's null driver to check overlapping sources, named stop, pause/resume, missing files and context restoration. Package CRC, embedded DLL/source equality and installed EXE/package hashes passed. No game was launched; visual and gameplay results await user testing.

## Cabin controls and engine audio batch — September 10, 2026

Installed the next removable `delorean.leaf` batch. Donor U/I wiper stalk, normal/fast/single/intermittent sweeps, J/K moving side windows and their switches, semicolon/apostrophe indicators, L hazards, accelerator/brake/clutch/handbrake animation, door warning and plutonium glovebox gauges are connected to the donor model frames. Window and wiper clips use the donor local offsets. Cabin state machines advance at the 30 fps donor baseline.

Engine audio now follows native engine on/off, ground contact, rear-wheel grip, forward acceleration, throttle release and folded wheels. Added start/stop, acceleration/deceleration with the 50 ms delayed stop, hover idle and fading landspeeder loop. Simultaneous sounds have independent local attachment points and gain; these points rotate with the car. Native ignition still determines engine state: this batch does not implement the donor custom throttle/exit ignition controller or turnover/failure behavior. The donor's commented-out engine_loop startup remains absent.

Deliberate compatibility adjustment: the donor pedal release helper zeroes a sufficiently negative delta and can leave a depressed pedal stuck. The native animation clamps release speed instead. Donor special weather-20 radiation is not reproduced. Existing flying particles, wheel-conversion timing and travel effects were not changed in this batch.

Validation: module build passed; standalone cabin tests passed at 15/30/60/144 fps, including engine acceleration restarting exactly when its delayed stop expires. Existing refuel/gauge tests passed. OpenAL null-driver tests additionally verified rotated local sound positions, gain/range and engine-context restoration. Archive CRC, all embedded sources and DLL, all referenced cabin model frames/sounds, and byte equality of 55 existing asset/particle/sound entries passed. Installed package SHA-256: `CFC1485737E8386469460E0294696933C94E6F206247A2299D450E8F5C327CBD`. Installed reVC.exe still matches the current executable build. No game was launched.

Remaining parity gaps include donor custom ignition and engine-turnover/failure states, further hover audio states, plutonium inventory/pickups, time-circuit switch behavior, and further particle/effect states. The donor turnover code has contradictory success/counter conditions; failure rules have not been invented. Do not describe this package as complete 1:1 parity.

## Head-only first person — September 10, 2026
Replaced whole-player render suppression with temporary zero scaling of PED_HEAD around the player body draw, restoring the exact matrix immediately afterward. Body/weapon rendering remains active; seated player visibility bypasses the standard bRenderPedInCar gate only in this camera. No damage flags changed. Build passed and executable installed/hash-verified (2B1AE595FE0972B0EF19C0A6F47EF9582329EBC9BBD51B336CA0C1BCE4E090AD). No game launched; body visibility, clipping and camera behavior await user testing.

User-requested U cycle: normal, fast, off; U also parks intermittent. Holding U cannot advance multiple states. Module built, cycle checks passed at 15/30/60/144 fps; package CRC passed, installed hash A382869F41EAA12ABB927E4F6ADCB3BB5FD18BF4F50FD5EC863462D35CF708BF. No game launched.

Signal controls updated: Left Shift+L left, Right Shift+L right, L alone hazards. Same combination toggles off; held L cannot repeat or change modes until released. Both shifts are treated like hazards. Build and 15/30/60/144 fps input/hold/release tests passed. Package installed and hash verified: 0EAD6373413B83CA8A247F20D29B89F1DCE780232192BC692C7EA03EB717F50A. No game launched.

## Shifter and seated pedal pose
Donor gear lever neutral/reverse/1-5 and wheel-speed RPM ratios now ported; hover uses donor speed formula and neutral lever. Verified actual DFF names shifterN/shifterR (uppercase). Tests cover targets, saturation and 15/30/60/144fps. User-requested native seated foot alignment uses a two-segment leg solver, moves right ankle between gas/brake, and follows pedal angles. This adjustment is not donor code or a certified visual fix. Only DeLorean driver in PED_DRIVING is affected. Optional LeafPedPose export; host backs up and restores the entire bone-matrix array around drawing, preserving normal pose and camera queries. Feet stay visible. Solver tests cover reach, preserved leg lengths, degenerate and opposite directions. Build and package CRC/source/DLL/asset/frame checks passed. No game launched. Installed EXE 231C97568CABDF178C585AD175B6ED0877E2AA714326BB9088FCF49FD0BEBEE8; leaf AE1AD07D55465DB4D4CBB684EEC10F824B634F6E5ECA6C8F36512429C6D05673. Rollback E:\Codex-work-backups\re3-camera-20260910\before-driver-pose-20260910-235251. Latest user-session logs before this update confirm pedal input reaches frames and gas rotates to 20 degrees then releases; appearance remains user-verified only.

## Hover underbody lighting, September 11
Ported donor UnderbodyLights.txt: bottomlights alpha moves by 15 per 30fps baseline tick; five chaserlights groups advance every 250ms in a 1250ms cycle when BTTF2 hover wheels are folded. Unfolding fades base light out and hides chasers. Donor engine flag is read but unused, so no new engine gate added. Build and 15/30/60/144fps fade/timing tests passed; referenced light frames verified in DFF; package CRC/source/DLL/unchanged asset checks passed. Installed hash 01E67B65F65A15DF32D5EC68102C51CA46DA1317BCEB3F296BA41F1E352B589A. No game launched; appearance unverified.

## Compass, horn stalk and overhead emergency light
Ported donor Compass.txt, Horn.txt and EmergencyLight.txt: compass rotates opposite heading, mapped horn shifts stalk from x=-.532 to -.522, P toggles overhead emergency light with attached emergency.wav at (.1,-.55,.55), range1. Handle animates 0-30 degrees in five-degree steps at 30fps baseline. P is scoped to focused occupied car; donor sound-finished gating retained. Build and 15/30/60/144fps handle tests passed; actual model names/sound verified, package CRC/source/DLL/unchanged assets checks passed. Installed hash 8F7C432AF47C375557694CD898E0638EAC48173D6CEC553D2BA896BA6C108B96. No game launched; visual behavior awaits user testing.

## Console clock
Ported active donor Clock.txt (streamed by Setup.txt): initialized from game time at spawn, independent 60-second minute ticks, 12-hour display, suppressed leading hour zero, half-second blinking colon, Bulova hour/minute angles when optional hands are visible. Clock remains independent of circuit power and game-clock jumps. Module pause guard freezes elapsed time; current travel is instantaneous, so a future multi-frame travel state will need donor TRAVEL freeze wiring. Donor C reset is intentionally not bound because user assigned C to hover; no replacement key invented. Build and 15/30/60/144fps rollover/format/blink tests passed. Model selectable digit and hand names verified; archive CRC/source/DLL/unchanged asset checks passed. Installed hash 80C1481C30107CB72BDBAD6967ACBBA78157BAD480E5617159FB371020AA1237. No game launched; appearance unverified.

## Digital speedometer and donor door sounds
Digital speedometer now uses donor 1.835 conversion, 88 mph cap and blank leading tens digit. Door state transitions play independent left/right door.wav voices at +/-1m and door_ice.wav when cold audio is active; repeated state frames do not retrigger. OpenAL supports named concurrent voices. Standalone speed/door/audio tests, build, package CRC, source/DLL and unchanged donor asset checks passed. Installed hash DA2F680EA1ADD823B390D25BBEAD080CE44698D2111BC9BE77F6679F31BE248A. No game launched.

## Compass and first-person body opacity
Compass now damps heading at the donor 30fps baseline and counteracts car pitch/roll. First-person body opacity uses temporary material alpha with vertex alpha blending, restored after drawing; debug +/- adjusts 10% steps, minimum10%, maximum100%. The earlier vehicle-space leg IK override is disabled to preserve the native seated pose. Standalone tests and full reVC/DeLorean builds passed. Package CRC, DLL/source and donor asset checks passed. Installed EXE DCEBBAED637D3A95E717F177C547B30537A12353E67871CAA0D93F6CECC19141 and leaf AE409BD895409AAD6726E0BFAAFE729AA497137DAB3BAA35E8C087E592DF7A45. No game launched. Cabin/headlight light beams remain pending.

## Buoy compass and first-person opacity fix
Compass pitch/roll now uses spring+damping velocity, giving a liquid buoy response instead of direct tilt snapping. First-person body opacity now sets material modulation geometry flags and alpha blend states around the draw, restoring material colors/states afterward. Debug module rebuilt; +/- changes opacity in 10% steps. Experimental twisted leg IK remains disabled. Offline cabin tests and full builds passed. Installed EXE D7CFA3257181243C21047736FB4B9944C9DE54552A5BC2B28F17260FC3A935C8, DeLorean 5F63CA1080EC5CBCABDC61DC76A2FF62CC6CA088E232DF29F12661EC64D102F5, debug 8A677241CCEBC98BD5CC85D4C3C0717778A34361128058A2D4D680BF5F7CF738. Rollback E:\\Codex-work-backups\\re3-camera-20260910\\before-buoy-alpha-20260912-023757. No game launched. Headlight/cabin beams remain next.

## Light beams
Added directional point-light beams from donor DeLorean lightFl/lightFr frames when headlights are on, and a short downward cabin beam from overheadconsoleelight while emergency light is active. Uses existing CPointLights per-frame pipeline and actual car basis, so beams follow position and orientation. Model/source/assets verified; build/package CRC passed; installed leaf hash 3343009E902641D3F034CD4B7272B016B9C3526A02E990CE4A79AF75B092C007. Visual intensity/occlusion remains unverified because no game testing was performed.
