# Swing Sprint VR

An F4SE VR plugin for **Fallout 4 VR**: run by swinging your arms, and walk at a different speed indoors and
outdoors. It works standalone with any VR controllers and any way of moving: the thumbstick, a walk-in-place device
such as the **KAT VR Loco S**, or a treadmill. It was made with a KAT Loco S and Meta Quest 2 controllers.

- **Movement speed by location** – separate walking speed for interiors and exteriors, so tight buildings and vaults
  don't feel too fast while the open world doesn't feel too slow.
- **Sprint by swinging your arms** – swing the controllers like running arms and the mod presses the game's sprint
  button for you. You keep walking with your feet; your arms decide when you run.
- **MCM VR menu** – every main setting can be changed in game from Mod Configuration Menu, applied immediately.
  Each slider has `[ - ]` and `[ + ]` buttons below it, since sliders are hard to drag in VR.
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

No locomotion hardware is needed. With a KAT Loco S, keep KAT Gateway running as usual: the mod adds the sprint on
top of the KAT input.

### Upgrading from KAT Locomotion VR (0.3.0 and older)

The mod was called *KAT Locomotion VR* before 0.4.0. Remove the old mod in MO2 before installing this one, so the
two plugins don't run at the same time. Your old settings are not carried over: set them again in MCM or in the new
INI. The old `Mods_Config\KATLocomotionVR` folder and `MCM\Settings\KATLocomotionVR.ini` can be deleted.

## Installation (Mod Organizer 2)

1. Download the `Swing Sprint VR - vX.Y.Z.7z` archive from [Releases](../../releases).
2. In MO2 choose **Install a new mod from an archive** and pick the archive.
3. Enable the mod in the left pane.
4. Start the game through MO2 with the F4SEVR loader (`f4sevr_loader.exe`).

Manual install: extract the archive into `Fallout 4 VR\Data`.

On first start the mod creates its settings file at
`Documents\My Games\Fallout4VR\Mods_Config\SwingSprintVR\SwingSprintVR.ini` and logs to
`Documents\My Games\Fallout4VR\F4SE\SwingSprintVR.log`.

## Settings

Changes are applied while the game is running, both from MCM and from the INI file.

**When MCM VR is installed, the values on its *Swing Sprint VR* page override the same values in the INI file.**
Settings MCM doesn't show stay in the INI.

VR sliders are hard to drag, so every slider on the MCM page has a `[ - ]` and a `[ + ]` button below it that
steps the value by the slider's step (5 for speeds, 0.1 or 0.05 for the swing values).

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
  `SpeedMult` actor value to the configured value when it differs. Right before every save (manual, quick or
  autosave) it puts SpeedMult back to 100, and the next frame applies the configured value again, so saves never
  hold the mod's speed and the mod can be removed at any time.
- **Swing detection:** the controllers' velocities are read from the OpenVR compositor's poses and taken relative to
  the headset, so head bobbing while walking in place doesn't count. The slower hand's speed (zero when the hands move
  the same way) is smoothed, and separate start and stop thresholds with a stop delay keep the sprint from flickering.
  Swinging is ignored while the Pip-Boy, pause, loading, workshop or dialogue menus are open.
- **Sprint button:** the plugin hooks `IVRSystem::GetControllerState[WithPose]` on top of any existing hooks and adds
  the sprint button press to the controller state the game reads. The physical controllers and any locomotion
  device input (such as KAT) are passed through unchanged.
- **MCM:** MCM VR stores its values in `Data\MCM\Settings\SwingSprintVR.ini` (defaults in
  `Data\MCM\Config\SwingSprintVR\settings.ini`). The plugin checks these files twice a second. The `[ - ]`/`[ + ]`
  buttons call the Papyrus global function `SwingSprintVR_MCM.StepFloat`, which steps the value with MCM's
  Papyrus API and refreshes the menu.

## Building from source

Requirements: Windows, Visual Studio 2026 (v145) or 2022 (v143) with C++ desktop development, CMake 4.2+,
[vcpkg](https://github.com/microsoft/vcpkg) (the one bundled with Visual Studio works) and Git.

```bash
git clone --recursive https://github.com/<you>/SwingSprintVR.git
cd SwingSprintVR
set VCPKG_ROOT=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\vcpkg
cmake --preset default
cmake --build --preset release
cmake --build --preset package
```

- The plugin is built to `build/Release/SwingSprintVR.dll`.
- The `package` preset makes the installable archive in `build/package/`.
- To copy the DLL to your game or MO2 mod folder on every build, create a `CMakeUserPresets.json` from
  `external/F4VR-CommonFramework/mod-template/CMakeUserPresets.json.template`.

The swing detection logic has standalone tests (no game needed), run from a Visual Studio developer prompt:

```bash
cd tests
cl /std:c++20 /EHsc /I..\src SwingDetectorTest.cpp ..\src\SwingDetector.cpp /Fe:swingtest.exe
swingtest.exe
```

The Papyrus script for the MCM buttons (`data/mod/Scripts/Source/User/SwingSprintVR_MCM.psc`) is compiled with the
Fallout 4 Creation Kit's Papyrus compiler; the compiled `.pex` is committed so a normal build doesn't need it. To
rebuild it, extract `Fallout 4\Data\Scripts\Source\Base\Base.zip` and get `MCM.psc` from the MCM download, then
from `data/mod/Scripts/Source/User`:

```bash
"<Fallout 4>\Papyrus Compiler\PapyrusCompiler.exe" SwingSprintVR_MCM.psc -f=<Base>\Institute_Papyrus_Flags.flg -i=.;<folder with MCM.psc>;<Base> -o=..\.. -release -final
```

### Project layout

| Path | Contents |
| --- | --- |
| `src/SwingSprintVR.*` | Plugin entry, speed by cell, sprint logic, Live stats |
| `src/SwingDetector.*` | Arm swing detection (pure logic, tested) |
| `src/SprintInjector.*` | OpenVR controller-state hook that presses the sprint button |
| `src/Config.*` | INI and MCM settings |
| `data/config/SwingSprintVR.ini` | Default INI, embedded into the DLL |
| `data/mod/` | Files shipped in the archive (MCM menu, Papyrus script for the - / + buttons) |
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

Swing Sprint VR on Fallout 4 VR:n F4SEVR-lisäosa: juoksu käsiä heiluttamalla, eri liikkumisnopeus sisä- ja
ulkotiloissa, MCM VR -valikko sekä reaaliaikainen Live stats -testinäkymä. Toimii itsenäisesti millä tahansa
VR-ohjaimilla ja liikkumistavalla, myös KAT Loco S:n kanssa. Nopeus ei tallennu save-tiedostoon. Asennus MO2:lla: *Install a new mod from an archive*, ja lisäksi tarvitaan
VR Address Library for F4SEVR. Asetukset muuttuvat pelin aikana MCM:stä tai INI-tiedostosta.
