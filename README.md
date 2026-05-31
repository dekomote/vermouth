<div align="center">

[![Build AppImage](https://github.com/dekomote/vermouth/actions/workflows/build-appimage.yml/badge.svg)](https://github.com/dekomote/vermouth/actions/workflows/build-appimage.yml)
[![Build DEB](https://github.com/dekomote/vermouth/actions/workflows/build-deb.yml/badge.svg)](https://github.com/dekomote/vermouth/actions/workflows/build-deb.yml)
[![Build RPM](https://github.com/dekomote/vermouth/actions/workflows/build-rpm.yml/badge.svg)](https://github.com/dekomote/vermouth/actions/workflows/build-rpm.yml)
[![Build Flatpak](https://github.com/dekomote/vermouth/actions/workflows/build-flatpak.yml/badge.svg)](https://github.com/dekomote/vermouth/actions/workflows/build-flatpak.yml)
[![Build Arch Package](https://github.com/dekomote/vermouth/actions/workflows/build-arch.yml/badge.svg)](https://github.com/dekomote/vermouth/actions/workflows/build-arch.yml)

</div>

<p align="center">
  <img src="assets/com.dekomote.vermouth.svg" width="128" height="128" alt="Vermouth logo">
</p>

<h1 align="center">Vermouth</h1>

<p align="center">A game and app launcher for Linux - native, Windows, and retro.<br>
KDE-first, lightweight, no frills.</p>

<p align="center">
  <img src="assets/screen1.png?t=1.8" alt="Game library with sidebar menu open" width="400"><br>
  <img src="assets/screen2.png?t=1.8" alt="Game settings" width="400">
  <img src="assets/screen4.png?t=1.8" alt="Right-click context menu with launch and Wine utility options" width="400">
  <img src="assets/screen3.png?t=1.8" alt="Settings dialog showing umu-launcher, Proton, SteamGridDB and RomM configuration" width="400"><br>
</p>

---

## Table of Contents

- [Table of Contents](#table-of-contents)
- [What it does](#what-it-does)
- [Requirements](#requirements)
- [Quick start](#quick-start)
- [How to use it](#how-to-use-it)
- [umu-launcher support](#umu-launcher-support)
- [SteamGridDB support](#steamgriddb-support)
- [Steam support](#steam-support)
- [RetroArch support](#retroarch-support)
- [RomM support](#romm-support)
- [Installing](#installing)
  - [Fedora and Nobara](#fedora-and-nobara)
  - [Bazzite](#bazzite)
  - [OpenSUSE Tumbleweed](#opensuse-tumbleweed)
  - [Ubuntu / Debian](#ubuntu--debian)
  - [Arch Linux / CachyOS](#arch-linux--cachyos)
  - [Flatpak](#flatpak)
  - [AppImage](#appimage)
- [Building from source](#building-from-source)
- [How it works](#how-it-works)
- [Flatpak Notes](#flatpak-notes)
- [Contributing](#contributing)
  - [Code](#code)
  - [Translations](#translations)
  - [AI](#ai)
- [Bug reporting and feature requests](#bug-reporting-and-feature-requests)

---

## What it does

Vermouth is a KDE-first launcher with four modes:

- **Windows games** - run `.exe` files with Proton or Wine, with umu-launcher support for full Steam Runtime compatibility
- **Native apps** - launch Linux binaries, `.desktop` entries, and AppImages directly
- **Steam games** - import your installed Steam library with one click and launch games directly via Steam
- **Retro games** - browse and launch your [RomM](https://github.com/rommapp/romm) library via RetroArch, with platform filtering, cover art, and ROM downloads; or add ROM files directly to your library

It works like Lutris, Heroic, or Bottles, but lighter and KDE-first - less buttons, checks and knobs, just the bare necessities.

**Proton / Wine**
- Searches for Proton versions from your Steam installation automatically, including GE-Proton
- Download the latest GE-Proton with one click if you don't have Steam
- Custom Proton builds go in `~/.local/share/vermouth/protons`
- Wine works too - point it at the binary and set a prefix folder
- Launch options with `%command%` placeholder, same as Steam (e.g. `mangohud %command%`, `GAMEID=12345 %command%`)
- Run a separate `.exe` inside an existing prefix (useful for installers and config tools)
- Run common Wine utilities - winecfg, regedit, winetricks
- Toggle HDR per-session on KDE - sets the required Proton environment variables automatically

**Library**
- Extracts icons from `.exe` files automatically (requires `icoutils`)
- Create start menu entries and desktop shortcuts that work without opening Vermouth
- Prevent the system from sleeping while a game is running
- SteamGridDB integration for icons, grid art, hero images, and logos

**UI**
- Big screen / Big Picture mode with full gamepad navigation

---

## Requirements

- Linux with **KDE Plasma 6** recommended - works on other DEs but without native Plasma styling
- **Qt 6.8+** and **KDE Frameworks 6**
- Proton or Wine for Windows games (GE-Proton can be downloaded from within Vermouth)
- RetroArch with cores installed for retro games
- [umu-launcher](https://github.com/Open-Wine-Components/umu-launcher) recommended for best Windows game compatibility (can be downloaded from within Vermouth)
- `icoutils` for automatic icon extraction from `.exe` files

Ubuntu 25.04 / Debian Trixie or newer are required on Debian-based systems due to the Qt 6.8+ dependency.

---

## Quick start

1. [Install Vermouth](#installing) for your distro
2. Click **Add Game** and browse for an `.exe`, binary, ROM, or `.desktop` file
3. Choose a runtime - Proton, Wine, RetroArch, or native
4. Double-click to launch

For Steam games, use **Menu → Import from Steam** if you don't want to search for game IDs.

---

## How to use it

**Windows / native games:** Click **Add Game**, browse for the `.exe` or binary, choose a runtime (Proton, Wine, or native), and launch from the grid. Optional fields can be omitted - the name and icon are inferred automatically (requires `icoutils` for `.exe` icons), and the prefix is set based on the game name.

The **Launch Options** field wraps the command with tools like `mangohud`, `gamescope`, or `gamemoderun`. Use `%command%` as the placeholder - if omitted, options are prepended automatically. You can also set environment variables here, e.g. `GAMEID=12345 %command%` to pass a Steam App ID to umu-launcher.

**Steam games:** Click **Menu → Import from Steam**. Vermouth scans your Steam library, shows all installed games, and lets you select which ones to import. Art is resolved from your local Steam cache and any gaps are filled from SteamGridDB automatically.

**RetroArch games:** Click **Add Game**, select **RetroArch** as the runtime, choose the ROM file and platform. Vermouth will pick the right core automatically or prompt you to select one.

**Retro games via RomM:** Configure your RomM server URL and API key in Settings. Switch to the **RomM** tab, and double-click a ROM to download and launch it. Cores can be overridden per game via right-click.

**Big Picture mode:** Click **Big Picture** in the sidebar or press the assigned shortcut to enter full-screen mode. The entire UI is navigable with a gamepad - L1/R1 switch tabs, L2 opens the platform picker in the RomM tab, and face buttons map to the primary actions.

In **Settings** you can:
- Configure [umu-launcher](#umu-launcher-support) for better game compatibility
- Set the default prefix folder and extra Proton scan paths
- Set your RomM server URL, API key, and ROM cache directory
- Configure your SteamGridDB api key etc.

---

## umu-launcher support

Vermouth supports [umu-launcher](https://github.com/Open-Wine-Components/umu-launcher), which runs Proton through the full Steam Runtime (pressure-vessel). This significantly improves game compatibility - especially for games with video cutscenes, media codecs, or anti-cheat. It is strongly recommended.

If `umu-run` is found in your `PATH` or configured in Settings, Vermouth will use it automatically for all Proton launches. You can also download it directly from Settings → umu-launcher.

---

## SteamGridDB support

Vermouth supports [SteamGridDB](https://www.steamgriddb.com) for fetching icons, grid and hero images and logos. You need an API key which you can get by registering an account with them and getting your API key [here](https://www.steamgriddb.com/profile/preferences/api).

---

## Steam support

Vermouth can import your installed Steam library in one click. Go to **Menu → Import from Steam**, select the games you want, and they will appear in your library. Launching them opens Steam directly to that game. Art is fetched from your local Steam cache automatically; any missing artwork is downloaded from SteamGridDB if you have an API key configured.

Steam is detected from all standard install locations, including native and Flatpak installs.

---

## RetroArch support

ROMs can be added to your library directly - no RomM required. Click **Add Game**, select **RetroArch** as the runtime, pick the ROM file and platform, and it will appear in your library alongside your other games.

Vermouth auto-detects RetroArch cores for each platform. You can override the core per entry from the right-click menu → **Change Core**.

---

## RomM support

Vermouth integrates with [RomM](https://github.com/rommapp/romm), a self-hosted retro game library manager. Point Vermouth at your RomM server URL and API key in Settings, and the RomM tab will let you browse platforms and ROMs, download them locally, and launch them with RetroArch.

ROMs are launched via RetroArch. Vermouth will auto-detect whether RetroArch is installed natively or as a Flatpak. You can assign a RetroArch core per game from the right-click menu. Cores must be installed separately in RetroArch before use.

---

## Installing

### Fedora and Nobara

Vermouth is available on [COPR](https://copr.fedorainfracloud.org/coprs/dekomote/Vermouth/):

```bash
sudo dnf copr enable dekomote/Vermouth
sudo dnf install vermouth
```

### Bazzite

```bash
sudo dnf5 copr enable dekomote/Vermouth
sudo rpm-ostree -y install vermouth
```

Also, you can download the latest package from the [releases page](https://github.com/dekomote/vermouth/releases/latest).

```bash
sudo dnf install ./vermouth-*.rpm
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
Download the latest deb package from the [releases page](https://github.com/dekomote/vermouth/releases/latest).

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

Install it from [flathub](https://flathub.org/en/apps/com.dekomote.vermouth)

```bash
flatpak install com.dekomote.vermouth
```

or

Download the latest flatpak package from the [releases page](https://github.com/dekomote/vermouth/releases/latest).

```bash
flatpak install ./vermouth-*.flatpak
```

> **Note:** The Flatpak sandbox restricts filesystem access to your home directory. If your games or Steam live elsewhere, you'll need to grant additional permissions. See [Flatpak Notes](#flatpak-notes).

### AppImage

Download `Vermouth-*.AppImage` from the [releases page](https://github.com/dekomote/vermouth/releases/latest), make it executable and run it:

```bash
chmod +x Vermouth-*.AppImage
./Vermouth-*.AppImage
```

---

For icon extraction from `.exe` files, install `icoutils` (provides `wrestool` and `icotool`).

---

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

---

## How it works

Games are stored in `~/.config/vermouth/apps.json`. When umu-launcher is available, Proton is launched through it with `PROTONPATH` and `STEAM_COMPAT_DATA_PATH` set. Without umu, Vermouth calls the `proton run` script directly, the same way Steam does. Wine games get `WINEPREFIX` set and the binary called directly.

---

## Flatpak Notes

When running Vermouth as a Flatpak, it is sandboxed and only has access to your home directory by default. If your games are stored outside your home folder, grant filesystem access using [Flatseal](https://flathub.org/apps/com.github.tchx84.Flatseal) or your desktop environment's application permissions settings. Add the relevant paths under **Filesystem** permissions.


HDR toggle on KDE requires the `org.freedesktop.Flatpak` talk permission — see [Flatpak RetroArch](#flatpak-retroarch) below for how to grant it.

### Flatpak RetroArch

Vermouth detects and launches RetroArch installed as a Flatpak (`org.libretro.RetroArch`). If Vermouth is also running as a Flatpak, an extra permission is needed to let it talk to the host and launch other Flatpak apps:

```bash
flatpak override --user --talk-name=org.freedesktop.Flatpak com.dekomote.vermouth
```

You can also add it via Flatseal under **Session Bus** → **Talks**.

---

## Contributing

Contributions are welcome. Please open a pull request on [GitHub](https://github.com/dekomote/vermouth).

### Code

Build from source (see [Building from source](#building-from-source)), make your changes, and open a pull request. Keep changes focused - one feature or fix per PR.

### Translations

Vermouth uses the KDE i18n system (gettext `.po` files). To add or update a translation:

1. Create a folder `po/<language_code>/` (e.g. `po/fr/` for French, `po/pt_BR/` for Brazilian Portuguese).
2. Copy `po/vermouth.pot` into it as `vermouth.po` (e.g. `po/fr/vermouth.po`).
3. Fill in the `msgstr` fields with your translations.
4. Open a pull request with your new folder.

To update an existing translation after new strings have been added:

```bash
sh po/Messages.sh       # regenerate vermouth.pot from source
sh po/update-po.sh      # merge new strings into all .po files
```

Then fill in any new empty `msgstr ""` entries in your `.po` file.

### AI

Development included assistance of AI tools.

---

## Bug reporting and feature requests

Please use the [issue tracker](https://github.com/dekomote/vermouth/issues) for bug reports and feature requests.
