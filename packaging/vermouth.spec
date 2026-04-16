Name:           vermouth
Version:        1.2.1
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
%else
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  qt6-qtquickcontrols2-devel
BuildRequires:  kf6-kirigami-devel
BuildRequires:  kf6-kcoreaddons-devel
BuildRequires:  kf6-ki18n-devel
BuildRequires:  kf6-qqc2-desktop-style
BuildRequires:  icoutils
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

%description
Vermouth is a no-frills lightweight game and application launcher that
lets you run Windows executables through Proton or Wine on KDE.

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

%changelog
* Tue Apr 14 2026 Dejan Noveski <deko@duck.com> - 1.2.1-1
- Initial COPR package
