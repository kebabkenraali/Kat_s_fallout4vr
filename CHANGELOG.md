# Changelog

## 0.4.0 - 2026-10-28

- Renamed from *KAT Locomotion VR* to **Swing Sprint VR**: the mod works standalone, without a KAT Loco S. Files are
  now `SwingSprintVR.dll`, `SwingSprintVR.ini`, the MCM page *Swing Sprint VR*. Remove the old mod before
  installing; settings are not carried over.
- The speed is no longer stored in save games: right before each save SpeedMult is put back to 100 and reapplied
  after, so the mod can be removed at any time.

## 0.3.0 - 2026-10-28

- MCM: every slider has - and + buttons below it that step the value, since VR sliders are hard to drag.

## 0.2.1 - 2026-10-28

- Removed unused framework textures and mesh from the package; it now ships only the plugin and the MCM menu.

## 0.2.0 - 2026-10-28

- MCM VR settings page: speeds, sprint button and mode, swing thresholds, Live stats. Values apply immediately and
  override the INI when MCM VR is installed.
- Live stats panel (`bShowLiveStats`, replaces `bShowSwingDebug`): location, speed target and current SpeedMult,
  measured movement speed, hand and swing speed with a threshold bar, sprint and button state.

## 0.1.0 - 2026-10-27

- Separate movement speed for interiors and exteriors.
- Sprint by swinging the controllers like running arms, pressing the game's sprint button (hold or tap mode).
- Hot-reloaded INI settings.
