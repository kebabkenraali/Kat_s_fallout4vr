# KAT Locomotion VR

An F4SE VR plugin for **Fallout 4 VR** made for playing with a **KAT VR Loco S** (or any walk-in-place / treadmill
locomotion) and **Meta Quest 2** controllers:

- **Movement speed by location** – separate walking speed for interiors and exteriors, so tight buildings and vaults
  don't feel too fast while the open world doesn't feel too slow.
- **Sprint by swinging your arms** – swing the controllers like running arms and the mod presses the game's sprint
  button for you. You keep walking with your feet; your arms decide when you run.
- **MCM VR menu** – every main setting can be changed in game from Mod Configuration Menu, applied immediately.
- **Live stats** – a real-time panel in front of you showing location, speed values, your measured movement speed,
  hand and swing speed against the thresholds, and sprint state, so you can see what each change does while you move.

> Not affiliated with KAT VR, Meta, Valve or Bethesda.

Made with Claude AI

## Requirements

| Requirement | Notes |
| --- | --- |
| Fallout 4 VR 1.2.72 | Steam version |
| [F4SEVR](https://f4se.silverlock.org/) 0.6.21 | Fallout 4 Script Extender for VR |
| [VR Address Library for F4SEVR](https://www.nexusmods.com/fallout4/mods/64879) | Required, the plugin won't load without it |
| [Mod Configuration Menu](https://www.nexusmods.com/fallout4/mods/21497), VR version | Optional, for the in-game settings menu |
| KAT Gateway | For the KAT Loco S. Not needed by the mod itself, any locomotion works |

## Installation (Mod Organizer 2)

1. Download the `KAT Locomotion VR - vX.Y.Z.7z` archive from [Releases](../../releases).
2. In MO2 choose **Install a new mod from an archive** and pick the archive.
3. Enable the mod in the left pane.
4. Start the game through MO2 with the F4SEVR loader (`f4sevr_loader.exe`).

Manual install: extract the archive into `Fallout 4 VR\Data`.

On first start the mod creates its settings file at
`Documents\My Games\Fallout4VR\Mods_Config\KATLocomotionVR\KATLocomotionVR.ini` and logs to
`Documents\My Games\Fallout4VR\F4SE\KATLocomotionVR.log`.

## Settings

Changes are applied while the game is running, both from MCM and from the INI file.

**When MCM VR is installed, the values on its *KAT Locomotion VR* page override the same values in the INI file.**
Settings MCM doesn't show stay in the INI.

### Movement speed by location

| INI key | Default | Description |
| --- | --- | --- |
| `bSpeedByCellEnabled` | `true` | Separate interior/exterior speed. `false` restores the default speed (100). |
| `fInteriorSpeedMult` | `80` | Speed multiplier in interiors (buildings, vaults, caves). 100 = game default. |
| `fExteriorSpeedMult` | `100` | Speed multiplier in exteriors (open world, Diamond City). |
| `bSpeedRefreshNudge` | `true` | Nudges carry weight so the game applies a new speed at once. |

### Sprint by swinging arms

| INI key | Default | Description |
| --- | --- | --- |
| `bSwingSprintEnabled` | `true` | Sprint while swinging the controllers like running arms. |
| `sSprintButton` | `offhand thumbstick` | The game's sprint button, `<hand> <button>`. Hands: `offhand`, `primary`, `left`, `right`. Buttons: `thumbstick`, `grip`, `trigger`, `a`, `b`. |
| `sSprintMode` | `hold` | `hold`: button held while swinging. `tap`: short press when swinging starts and stops (if the game toggles sprint). |
| `fSprintTapDuration` | `0.15` | Length of one press in tap mode, seconds. |
| `bRequireAlternatingArms` | `true` | Only count swings where the hands move in opposite directions. |
| `fSwingStartSpeed` | `1.4` | Hand speed (m/s, relative to the headset) that starts the sprint. |
| `fSwingStopSpeed` | `0.8` | Hand speed below which the sprint stops. Keep lower than the start speed. |
| `fSwingSmoothing` | `0.25` | Smoothing of the swing speed, seconds. Higher = steadier, slower to react. |
| `fSprintStopDelay` | `0.4` | How long the swing must stay slow before the sprint stops, seconds. |

### Testing

| INI key | Default | Description |
| --- | --- | --- |
| `bShowLiveStats` | `false` | Show the Live stats panel. |

The **Live stats** panel shows, updated every frame:

- where the settings come from (MCM or INI)
- interior or exterior
- the speed target and the game's current SpeedMult (orange while they differ)
- your measured movement speed in m/s, from the player's position change
- left and right hand speed
- the smoothed swing speed and a bar `[####..|...|...]` where the `|` marks are the stop and start thresholds
- whether the sprint is on and whether the mod is pressing the sprint button

### Tuning tips

- The sprint starts while just walking → raise `fSwingStartSpeed`.
- You have to swing too hard → lower `fSwingStartSpeed`.
- The sprint drops between swings → raise `fSprintStopDelay` or `fSwingSmoothing`.
- The sprint doesn't start at all although *Sprint: ON* shows → the game's sprint button is a different one, change
  `sSprintButton`; if it starts and stops right away, use `sSprintMode = tap`.

## How it works

- **Speed:** every frame the plugin checks whether the player's cell is an interior and sets the player's base
  `SpeedMult` actor value to the configured value when it differs. The value is stored in the save, so before
  uninstalling set `bSpeedByCellEnabled = false` and save once to return to 100.
- **Swing detection:** the controllers' velocities are read from the OpenVR compositor's poses and taken relative to
  the headset, so head bobbing while walking in place doesn't count. The slower hand's speed (zero when the hands move
  the same way) is smoothed, and separate start and stop thresholds with a stop delay keep the sprint from flickering.
  Swinging is ignored while the Pip-Boy, pause, loading, workshop or dialogue menus are open.
- **Sprint button:** the plugin hooks `IVRSystem::GetControllerState[WithPose]` on top of any existing hooks and adds
  the sprint button press to the controller state the game reads. The physical controllers and the KAT input are
  passed through unchanged.
- **MCM:** MCM VR stores its values in `Data\MCM\Settings\KATLocomotionVR.ini` (defaults in
  `Data\MCM\Config\KATLocomotionVR\settings.ini`). The plugin checks these files twice a second.

## Building from source

Requirements: Windows, Visual Studio 2026 (v145) or 2022 (v143) with C++ desktop development, CMake 4.2+,
[vcpkg](https://github.com/microsoft/vcpkg) (the one bundled with Visual Studio works) and Git.

```bash
git clone --recursive https://github.com/<you>/KATLocomotionVR.git
cd KATLocomotionVR
set VCPKG_ROOT=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\vcpkg
cmake --preset default
cmake --build --preset release
cmake --build --preset package
```

- The plugin is built to `build/Release/KATLocomotionVR.dll`.
- The `package` preset makes the installable archive in `build/package/`.
- To copy the DLL to your game or MO2 mod folder on every build, create a `CMakeUserPresets.json` from
  `external/F4VR-CommonFramework/mod-template/CMakeUserPresets.json.template`.

The swing detection logic has standalone tests (no game needed), run from a Visual Studio developer prompt:

```bash
cd tests
cl /std:c++20 /EHsc /I..\src SwingDetectorTest.cpp ..\src\SwingDetector.cpp /Fe:swingtest.exe
swingtest.exe
```

### Project layout

| Path | Contents |
| --- | --- |
| `src/KATLocomotionVR.*` | Plugin entry, speed by cell, sprint logic, Live stats |
| `src/SwingDetector.*` | Arm swing detection (pure logic, tested) |
| `src/SprintInjector.*` | OpenVR controller-state hook that presses the sprint button |
| `src/Config.*` | INI and MCM settings |
| `data/config/KATLocomotionVR.ini` | Default INI, embedded into the DLL |
| `data/mod/` | Files shipped in the archive (MCM config, framework assets) |
| `external/F4VR-CommonFramework` | Git submodule: the framework and CommonLibF4VR |
| `tests/` | Standalone tests |

## Credits

- [F4VR Common Framework](https://github.com/ArthurHub/F4VR-CommonFramework) by Arthur T (GPL-3.0-or-later)
- [CommonLibF4VR](https://github.com/ArthurHub/CommonLibF4VR), based on CommonLibF4 by Ryan McKenzie and contributors (MIT)
- [F4SEVR](https://f4se.silverlock.org/) by the F4SE team
- [OpenVR](https://github.com/ValveSoftware/openvr) by Valve (BSD-3-Clause)
- [MCM](https://www.nexusmods.com/fallout4/mods/21497) by the MCM team

Developed with the help of Claude Code (Anthropic).

## License

[GPL-3.0-or-later](LICENSE). The mod links the F4VR Common Framework statically, which is GPL-3.0-or-later, so the
mod is released under the same license.

---

## Suomeksi

KAT Locomotion VR on Fallout 4 VR:n F4SEVR-lisäosa KAT Loco S -kävelylaitteelle ja Quest 2 -ohjaimille:
eri liikkumisnopeus sisä- ja ulkotiloissa, juoksu käsiä heiluttamalla, MCM VR -valikko sekä reaaliaikainen
Live stats -testinäkymä. Asennus MO2:lla: *Install a new mod from an archive*, ja lisäksi tarvitaan
VR Address Library for F4SEVR. Asetukset muuttuvat pelin aikana MCM:stä tai INI-tiedostosta.
