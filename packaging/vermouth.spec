Name:           vermouth
Version:        1.8.5
Release:        1%{?dist}
Summary:        A no-frills Wine/Proton game launcher for KDE
License:        MIT
URL:            https://github.com/dekomote/vermouth
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.20
BuildRequires:  extra-cmake-modules

%if 0%{?suse_version}
BuildRequires:  gcc-c++
BuildRequires:  qt6-base-devel
BuildRequires:  qt6-declarative-devel
BuildRequires:  kf6-kirigami-devel
BuildRequires:  kf6-kcoreaddons-devel
BuildRequires:  kf6-ki18n-devel
BuildRequires:  kf6-qqc2-desktop-style-devel
BuildRequires:  icoutils
BuildRequires:  SDL2-devel
%else
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  qt6-qtquickcontrols2-devel
BuildRequires:  kf6-kirigami-devel
BuildRequires:  kf6-kcoreaddons-devel
BuildRequires:  kf6-ki18n-devel
BuildRequires:  kf6-qqc2-desktop-style
BuildRequires:  icoutils
BuildRequires:  SDL2-devel
%endif

%if 0%{?suse_version}
Requires:       libQt6Core6
Requires:       libQt6Gui6
Requires:       libQt6Network6
Requires:       libQt6DBus6
Requires:       libQt6Widgets6
Requires:       libQt6Qml6
Requires:       libQt6Quick6
Requires:       libQt6QuickControls2-6
Requires:       kf6-kirigami
%else
Requires:       qt6-qtbase
Requires:       qt6-qtdeclarative
Requires:       qt6-qtquickcontrols2
Requires:       kf6-kirigami
%endif
Recommends:     icoutils
Recommends:     SDL2

%description
Vermouth is a KDE-first game and application launcher. Run native Linux apps
and games, Windows titles through Proton or Wine, and retro games via your
RomM library with RetroArch - all from one place.

%prep
%autosetup

%build
%cmake
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%{_bindir}/vermouth
%{_datadir}/applications/com.dekomote.vermouth.desktop
%{_datadir}/icons/hicolor/scalable/apps/com.dekomote.vermouth.svg
%{_datadir}/metainfo/com.dekomote.vermouth.metainfo.xml
%{_datadir}/locale/*/LC_MESSAGES/vermouth.mo

%changelog
* Thu Jun 4 2026 Dejan Noveski <deko@duck.com> - 1.8.5-1
- Gog games import
* Sun May 31 2026 Dejan Noveski <deko@duck.com> - 1.8.4-1
- Flapak and shortcut fixes
* Wed May 27 2026 Dejan Noveski <deko@duck.com> - 1.8.3-1
- Default grid, welcome screen, fallback covers
* Fri May 15 2026 Dejan Noveski <deko@duck.com> - 1.8.2-1
- Welcome screen
* Fri May 15 2026 Dejan Noveski <deko@duck.com> - 1.8.1-1
- Wine works like proton now.
* Sat May 9 2026 Dejan Noveski <deko@duck.com> - 1.7.5-1
- Bugfixes
* Thu May 7 2026 Dejan Noveski <deko@duck.com> - 1.7.4-1
- Glow up
* Wed May 6 2026 Dejan Noveski <deko@duck.com> - 1.7.2-1
- Fixes for Umu, dark mode and icons
* Mon May 4 2026 Dejan Noveski <deko@duck.com> - 1.7.1-1
- Steam runner, refactors
* Mon May 4 2026 Dejan Noveski <deko@duck.com> - 1.6.3-1
- Fix for winetricks
* Mon May 4 2026 Dejan Noveski <deko@duck.com> - 1.6.2-1
- Background regression fix
* Sun May 3 2026 Dejan Noveski <deko@duck.com> - 1.6.1-1
- RomM integration, dark mode fixes
* Thu Apr 30 2026 Dejan Noveski <deko@duck.com> - 1.5.1-1
- SteamGridDB integration, fixes
* Mon Apr 27 2026 Dejan Noveski <deko@duck.com> - 1.4.1-1
- Launch native apps
* Sat Apr 25 2026 Dejan Noveski <deko@duck.com> - 1.3.5-1
- Lights off mode, Big Picture mode
* Tue Apr 21 2026 Dejan Noveski <deko@duck.com> - 1.3.4-1
- Single prefix fix, HDR Enable fix for full gamut monitors
* Mon Apr 20 2026 Dejan Noveski <deko@duck.com> - 1.3.3-1
- Single prefix support, known exe launch, Stop button, HDR disable, translations
* Fri Apr 17 2026 Dejan Noveski <deko@duck.com> - 1.3.2-1
- UMU prefix fixed, added single instance support
* Fri Apr 17 2026 Dejan Noveski <deko@duck.com> - 1.3.1-1
- Update to 1.3.1 - HDR and UMU
* Tue Apr 14 2026 Dejan Noveski <deko@duck.com> - 1.2.1-1
- Initial COPR package
