# Vermouth — Full User Testing Suite

A manual QA checklist covering the whole application. Work top to bottom, or jump
to the section relevant to your change.

## 1. First run & onboarding
- [ ] Fresh profile launches the Welcome screen (empty library, tips enabled).
- [ ] Download a Proton runtime from Welcome → it appears in the RuntimePicker.
- [ ] Download umu-launcher from Welcome → progress shows, completes, hint disappears.
- [ ] "Add a Game" / "Import from Steam" buttons work from Welcome.
- [ ] "Don't show me tips" persists; "Get Started" goes to Games.
- [ ] Relaunch after first run → Welcome does **not** reappear.

## 2. Library views
- [ ] Switch Icon / Cover art / Hero views (header dropdown and footer buttons) — all render.
- [ ] Toggle "Show names" on/off.
- [ ] Zoom slider + zoom in/out buttons resize cards (0.8–1.8); persists across restart.
- [ ] "Alt. grid background" setting changes the grid backdrop.
- [ ] Empty-library and empty-search placeholders show appropriate text.

## 3. Adding games
- [ ] **Add a Game**: pick exe, name, runtime → saved and appears in grid.
- [ ] **Run a Standalone EXE** (no library entry) launches.
- [ ] **Drag & drop** an `.exe` onto the window → "Run Standalone / Add to Library" prompt.
- [ ] Open exe via CLI/file association → if known, launches; if not, prompt appears.
- [ ] Add with each runtime: **Proton**, **Wine**, **Native**.
- [ ] Validation: Proton/Wine require a selected version (error shown if missing).

## 4. Editing / deleting
- [ ] Edit an existing game (name, runtime, paths, options) → persists.
- [ ] **Delete app only** (native/steam/retroarch): OK/Cancel; prefix preserved.
- [ ] **Delete app + prefix**: shows prefix path, requires typing `DELETE`, button disabled until exact match; reopening resets field.
- [ ] Deleting removes desktop shortcuts too.
- [ ] `Delete` / `Shift+Delete` keyboard shortcuts on a selected card behave correctly.

## 5. Runtime management (RuntimePicker)
- [ ] Select runtime type switches between Proton/Wine version pickers.
- [ ] Download **GE Proton** and **Proton-CachyOS** (menu) → progress, appears after refresh.
- [ ] Download Wine builds (Wow64 / regular / TKG / TKG-Wow64).
- [ ] Refresh buttons re-scan Proton/Wine.
- [ ] "Open Vermouth Proton/Wine folder" opens the right path.
- [ ] umu-launcher: auto-detect, manual pick, download; resulting binary is executable; re-download overwrites.

## 6. Prefixes
- [ ] Default prefix parent folder honored for new prefixes.
- [ ] Shared default Proton prefix vs auto-per-game.
- [ ] Shared default Wine prefix vs auto-per-game (`wines/` subfolder).
- [ ] Launch a Proton/Wine game once → prefix is created.
- [ ] "Prefix not ready" dialog appears when launching a prefix-shortcut before first run.

## 7. Launching
- [ ] Launch Proton, Wine, Native games successfully.
- [ ] Running indicator (play→stop) reflects state; Stop terminates.
- [ ] "Launched: X" / "Error launching: X" notifications appear.
- [ ] Per-game **launch options** with `%command%` placeholder work.
- [ ] Global **env vars** (Settings) applied; per-game options override.
- [ ] Enable logging → log file written to AppData/logs.
- [ ] Launch options with spaces/quotes in paths quote correctly (no breakage).

## 8. Steam & GOG import
- [ ] **Steam import**: detects installed Steam, lists games, imports selection with art/metadata.
- [ ] Imported Steam game launches (steam app id / runner shown in status bar).
- [ ] **GOG import**: scans, lists, imports; imported game launches.
- [ ] Re-running import doesn't duplicate entries.

## 9. SteamGridDB artwork
- [ ] Set API key in Settings.
- [ ] Auto-download art on add (when enabled): icon/grid/hero/logo fetched.
- [ ] Manual art picker: search, choose icon/grid/hero/logo, applied to card.
- [ ] Re-downloading hero overwrites existing (no stale image).
- [ ] "Auto downloading: X" status appears in footer during fetch.

## 10. RomM integration
- [ ] Configure server URL + API key in Settings → RomM nav item appears.
- [ ] RomM page loads platforms; platform combo populates; "All Platforms" entry present.
- [ ] Select a platform → ROMs list loads; ROM count in footer.
- [ ] Search filters ROMs.
- [ ] Download a ROM → progress; cached under ROM cache folder.
- [ ] Launch a ROM via RetroArch; **core picker** appears when core unknown, and choice is remembered (per-platform and per-game).
- [ ] RetroArch auto-detected (PATH or Flatpak) or set manually.

## 11. Navigation & sidebar
- [ ] Sidebar lists Games + RomM (RomM only when configured).
- [ ] Settings/About/Welcome open as pages with back arrow + title.
- [ ] Back returns to the previous page correctly.
- [ ] Pin/unpin sidebar (footer pin button); width is draggable and persists.
- [ ] Unpinned: hamburger shows on every page and opens the drawer.
- [ ] Drawer actions (Add Game, Run EXE, Import Steam/GOG, Sleep, HDR, Lights, Big Picture, Settings, About, Quit) all work.

## 12. Search
- [ ] Search on Games filters the grid live; clearing restores all.
- [ ] Switching pages clears the query.

## 13. Big Picture mode
- [ ] Enter via drawer action / F11 / configured gamepad button → fullscreen, Lights Out, larger UI.
- [ ] Exit restores previous windowed state, Lights Out value, and scale factor.

## 14. Gamepad
- [ ] D-pad navigates the grid; A launches; B clears selection / closes drawer/popup.
- [ ] Y focuses search (on-screen keyboard shows).
- [ ] Select opens/closes the drawer (modal only).
- [ ] L1/R1 cycle nav pages; R2 opens the RomM platform combo.
- [ ] Configured fullscreen combo (Guide / Select+L2 / L3+R3) toggles Big Picture.
- [ ] **Sidebar pinned**: B/Y do not collapse the sidebar.

## 15. HDR
- [ ] HDR toggle visible only when supported; toggling on/off works (footer + drawer).
- [ ] State reflected in both footer button and drawer action.

## 16. Sleep inhibition
- [ ] Toggle Prevent/Allow Sleep (footer + drawer); icon reflects state.
- [ ] Native: `systemd-inhibit --list` shows a Vermouth sleep inhibitor while active.
- [ ] No auto-suspend while active; inhibitor cleared when off.
- [ ] State restored on next launch.

## 17. Appearance / Lights Out
- [ ] Toggle Lights Out; whole UI (incl. Settings & About pages) inverts and stays readable.
- [ ] Custom background color picker + Reset (`#2A2E32`) applies live.
- [ ] Pinning is disabled while in Lights Out (and restored after).
- [ ] Footer divider line visible in both modes.

## 18. Settings persistence
- [ ] Every Settings field saves on **Save** with a "Settings saved" toast; page stays.
- [ ] Reopen Settings → values reload correctly.
- [ ] Extra Proton scan paths add/remove and affect detection.
- [ ] Show Tips, Auto-download Art, gamepad fullscreen button all persist.

## 19. Status bar / footer
- [ ] Selecting a game shows "Runner - exe/appid path" (home path abbreviated to `~`).
- [ ] RomM page shows ROM count / status text.
- [ ] Status clears on non-games pages.

## 20. Window & single instance
- [ ] Window size persists across restarts (windowed only).
- [ ] Launching a second instance / opening an exe forwards to the running window and raises it.

## 21. Flatpak build (if applicable)
- [ ] "Talk to the host" hint shows in Settings + Welcome; Copy button copies the override command.
- [ ] HDR and RetroArch work after granting the permission; sleep inhibit works via portal.
- [ ] Hint is hidden on a native build.
