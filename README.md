# Vinz Mod Browser

A small Geometry Dash Geode mod that adds a button to the main menu and opens Geode's official in-game mod index.

## Compatibility target
- Geode SDK: v5.11.2
- Geometry Dash: Android 2.2081
- Build target: Android 64-bit (ARM64)

The official Geode index performs compatibility checks and manages mod installation, including dependencies. This mod delegates downloads and installation to Geode instead of downloading arbitrary packages itself.

## Build
The GitHub Actions workflow builds the Android64 package and uploads it as an artifact.
