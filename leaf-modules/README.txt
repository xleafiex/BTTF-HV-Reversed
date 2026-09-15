reVC Leaf modules

Each .leaf archive is optional and removable while the game is closed:
- delorean.leaf: donor DeLorean model, collision, textures, sounds, and native controls. Press `.` to spawn; `Enter` uses the normal game door/entry animation; `C` toggles hover; `+` toggles circuits; `F7` switches the BTTF I/II equipment; `F8` refuels; enter `MMDDYYYYHHMM` then `-` to set a destination.
- graphics.leaf: native GL3 water swell/lighting, sky/cloud pass, and weather controls.
- debug.leaf: supplies the release freeroam SCM and F10 head-mounted first-person camera. Development-only F9 fire preview and body-opacity controls are not included in the release archive.
- ragdoll.leaf: Bullet whole-body prototype for the player and nearby peds; F6 toggles it.

The 64-bit reVC build does not load the donor's original 32-bit CLEO/ASI binaries.
Anti-aliasing remains provided by reVC itself. Remove an archive to disable that module.
