<p align="center">
  <img src="assets/vermouth.svg" width="128" height="128" alt="Vermouth logo">
</p>

<h1 align="center">Vermouth</h1>

<p align="center">A no-frills game (or any Windows exe) launcher for Linux.<br>
Point it at Windows executables and run them with Proton or Wine - no Steam required.</p>

<p align="center">
  <img src="assets/screenshot.png" alt="Vermouth screenshot" width="700">
</p>

## What it does

Vermouth keeps a list of your games and applications, paired with a Proton or Wine version. Double-click to launch. That's pretty much it. It does everything Lutris or Heroic does, except it's much lighter, built on Qt and it lets other apps manage the compatibility tools (e.g. Steam, Protonup-qt etc.).

- Picks up Proton versions from your Steam installation automatically, including custom ones like GE-Proton from compatibilitytools.d and across multiple Steam library folders
- Wine works too - just point it at the Wine binary and set a prefix folder
- Extracts icons from .exe files so the grid actually looks nice
- Launch options with `%command%` placeholder, same as Steam (e.g. `mangohud %command%`)
- Run a separate .exe inside an existing prefix (useful for installers, config tools, etc.)
- Create start menu entries or desktop shortcuts for individual games
- Can be launched from .desktop files directly via CLI flags, so shortcuts work without the main window

## Building

You need Qt 6 and CMake. On Fedora:

```
sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel cmake gcc-c++
```

Then:

```
cmake -B build
cmake --build build
./build/vermouth
```

For icon extraction from .exe files, install `icoutils` (provides `wrestool` and `icotool`).

```
sudo dnf install icoutils
```

I'll follow up with build instructions for other distros as the code matures.

## How it works

Games are stored in `~/.config/vermouth/apps.json`. Proton is launched the same way Steam does it, by calling the `proton run` script with `STEAM_COMPAT_DATA_PATH` set to your prefix. Wine games just get `WINEPREFIX` set and the binary called directly.

The launch options field lets you wrap the command with tools like mangohud, gamescope, or gamemoderun. Use `%command%` as the placeholder for where the actual game command goes. If you leave out `%command%`, your options get prepended automatically.

## AI Disclaimer

The code has been developed, reviewed and tested by a human. However, development included assistance of AI tools, so keep that in mind.
