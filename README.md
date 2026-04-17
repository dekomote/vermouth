[![Build AppImage](https://github.com/dekomote/vermouth/actions/workflows/build-appimage.yml/badge.svg)](https://github.com/dekomote/vermouth/actions/workflows/build-appimage.yml)
[![Build DEB](https://github.com/dekomote/vermouth/actions/workflows/build-deb.yml/badge.svg)](https://github.com/dekomote/vermouth/actions/workflows/build-deb.yml)
[![Build RPM](https://github.com/dekomote/vermouth/actions/workflows/build-rpm.yml/badge.svg)](https://github.com/dekomote/vermouth/actions/workflows/build-rpm.yml)
[![Build Flatpak](https://github.com/dekomote/vermouth/actions/workflows/build-flatpak.yml/badge.svg)](https://github.com/dekomote/vermouth/actions/workflows/build-flatpak.yml)
[![Build Arch Package](https://github.com/dekomote/vermouth/actions/workflows/build-arch.yml/badge.svg)](https://github.com/dekomote/vermouth/actions/workflows/build-arch.yml)


<p align="center">
  <img src="assets/com.dekomote.vermouth.svg" width="128" height="128" alt="Vermouth logo">
</p>

<h1 align="center">Vermouth</h1>

<p align="center">A no-frills game (or any Windows exe) launcher for KDE.<br>
Point it at Windows executables and run them with Proton or Wine.</p>

<p align="center">
  <img src="assets/screen1.png?t=1.0" alt="Vermouth screenshot" width="400">
  <img src="assets/screen2.png?t=1.0" alt="Vermouth screenshot" width="400"><br>
  <img src="assets/screen3.png?t=1.0" alt="Vermouth screenshot" width="400">
  <img src="assets/screen4.png?t=1.0" alt="Vermouth screenshot" width="400">
</p>

## What it does

Vermouth keeps a list of your games and applications, paired with a Proton or Wine version. Double-click to launch. That's pretty much it.
It works like Lutris, Heroic, Fagus or Bottles, but:

- It's KDE first (written in Qt/Qml and using Kirigami)
- It tries to be lighter and easier to use - less buttons, checks and knobs, just the bare necessities.

Additionally:

- It searches for Proton versions from your Steam installation automatically, including custom ones like GE-Proton
- You can download the latest GE-Proton release with one click if you don't have Steam
- You can place custom Proton builds in its local folder (usually `~/.local/share/vermouth/protons`, there's a button to open it)
- Wine works too - just point it at the Wine binary and set a prefix folder
- It will try to extract icons from .exe files so the grid actually looks nice - install `icoutils` to enable this
- You can define launch options with `%command%` placeholder, same as in Steam (e.g. `mangohud %command%`, `GAMEID=12345 %command%`)
- You can run a separate .exe inside an existing prefix (useful for installers, config tools, etc.)
- You can run common Wine utilities in the prefixes - winecfg, regedit, winetricks
- You can create start menu entries and desktop shortcuts for individual games, and they work without opening the application window
- You can prevent the system from sleeping while a game is running
- HDR can be toggled per-session on KDE - enabling it also sets the required Proton environment variables automatically

### umu-launcher support

Vermouth supports [umu-launcher](https://github.com/Open-Wine-Components/umu-launcher), which runs Proton through the full Steam Runtime (pressure-vessel). This significantly improves game compatibility - especially for games with video cutscenes, media codecs, or anti-cheat. It is strongly recommended.

If `umu-run` is found in your `PATH` or configured in Settings, Vermouth will use it automatically for all Proton launches. You can also download it directly from Settings → umu-launcher.

## Installing

Download the latest package from the [releases page](https://github.com/dekomote/vermouth/releases/latest).

### Fedora and Nobara

```bash
sudo dnf install ./vermouth-*.rpm
```

Vermouth is also available on [COPR](https://copr.fedorainfracloud.org/coprs/dekomote/vermouth/):

```bash
sudo dnf copr enable dekomote/vermouth
sudo dnf install vermouth
```

### Bazzite

```bash
sudo dnf5 copr enable dekomote/vermouth
sudo rpm-ostree -y install vermouth
```

### OpenSUSE Tumbleweed

Install via COPR:

```bash
sudo zypper addrepo https://copr.fedorainfracloud.org/coprs/dekomote/Vermouth/repo/opensuse-tumbleweed/dekomote-Vermouth-opensuse-tumbleweed.repo
sudo zypper install vermouth
```

Or install the RPM from the [releases page](https://github.com/dekomote/vermouth/releases/latest):

```bash
sudo zypper install ./vermouth-opensuse-*.rpm
```

### Ubuntu / Debian

Requires Ubuntu 25.04 / Debian Trixie or newer (Qt 6.8+ and KF6 are required).

```bash
sudo apt install ./vermouth-*.deb
```

### Arch Linux / CachyOS

Install from the [AUR](https://aur.archlinux.org/packages/vermouth):

```bash
yay -S vermouth
```

Or install the package from the [releases page](https://github.com/dekomote/vermouth/releases/latest):

```bash
sudo pacman -U vermouth-*-arch.pkg.tar.zst
```

Or build from the included PKGBUILD:

```bash
cd packaging && makepkg -si
```

### Flatpak

```bash
flatpak install ./vermouth-*.flatpak
```

Or install from Flathub (once available):

```bash
flatpak install flathub com.dekomote.vermouth
```

See [FLATPAK NOTES](#flatpak-notes) for filesystem permissions required to access your games and Steam installation.

### AppImage

Download `Vermouth-*.AppImage`, make it executable and run it:

```bash
chmod +x Vermouth-*.AppImage
./Vermouth-*.AppImage
```

---

For icon extraction from .exe files, install `icoutils` (provides `wrestool` and `icotool`).

I have limited testing capabilities at the moment, so please report any bugs you find.

## Building from source

You need Qt 6.8+, KDE Frameworks 6, and CMake.

**Fedora:**
```bash
sudo dnf install cmake gcc-c++ extra-cmake-modules qt6-qtbase-devel qt6-qtdeclarative-devel \
  qt6-qtquickcontrols2-devel kf6-kirigami-devel kf6-kcoreaddons-devel kf6-ki18n-devel \
  kf6-qqc2-desktop-style icoutils
```

**Ubuntu / Debian:**
```bash
sudo apt install build-essential cmake extra-cmake-modules qt6-base-dev qt6-declarative-dev \
  qt6-tools-dev-tools libkirigami-dev libkf6coreaddons-dev libkf6i18n-dev \
  libkf6qqc2desktopstyle-dev icoutils
```

**Arch Linux:**
```bash
sudo pacman -S --needed base-devel cmake ninja extra-cmake-modules qt6-base qt6-declarative \
  kirigami ki18n kcoreaddons qqc2-desktop-style icoutils
```

Then inside the root folder of the project:

```bash
cmake -B build
cmake --build build
./build/bin/vermouth
```

## How to use it

Open the app, click **Add App/Game**, browse for the .exe file, choose a Proton version from the dropdown (click **Download GE Proton** if you don't have any), and launch from the grid.

Optional fields can be omitted - the name and icon will be inferred from the exe (requires `icoutils`), and the prefix will be set automatically based on the game name and your default prefix folder.

The **Launch Options** field lets you wrap the command with tools like `mangohud`, `gamescope`, or `gamemoderun`. Use `%command%` as the placeholder for where the actual game command goes - if you leave it out, your options are prepended automatically. You can also set environment variables here, e.g. `GAMEID=12345 %command%` to pass a Steam App ID to umu-launcher for game-specific Proton fixes.

In **Settings** you can:
- Configure [umu-launcher](#umu-launcher-support) for better game compatibility
- Set the default prefix folder
- Add extra folders to scan for Proton installations

## FLATPAK NOTES

When running Vermouth as a Flatpak, it is sandboxed and cannot access your filesystem by default. You need to grant it access to the folder(s) containing your games using [Flatseal](https://flathub.org/apps/com.github.tchx84.Flatseal) or your desktop environment's application permissions settings. Add the relevant paths under **Filesystem** permissions.

To detect Proton versions from your Steam installation, add `~/.steam:ro` and `~/.local/share/Steam:ro`.

To create desktop shortcuts for your games, add `xdg-desktop` to filesystem permissions.

## How it works

Games are stored in `~/.config/vermouth/apps.json`. When umu-launcher is available, Proton is launched through it with `PROTONPATH` and `STEAM_COMPAT_DATA_PATH` set. Without umu, Vermouth calls the `proton run` script directly, the same way Steam does. Wine games get `WINEPREFIX` set and the binary called directly.

## AI Disclaimer

The code has been developed, reviewed and tested by a human. However, development included assistance of AI tools, so keep that in mind.
