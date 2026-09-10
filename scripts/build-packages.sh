#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTPUT_DIR="${OUTPUT_DIR:-$PROJECT_DIR/dist}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

PASSED=0
FAILED=0
SKIPPED=0

pass()   { echo -e "${GREEN}PASS${RESET}  $1"; PASSED=$((PASSED + 1)); }
fail()   { echo -e "${RED}FAIL${RESET}  $1"; FAILED=$((FAILED + 1)); }
skip()   { echo -e "${YELLOW}SKIP${RESET}  $1"; SKIPPED=$((SKIPPED + 1)); }

CONTAINER_RT="${CONTAINER_RT:-podman}"
TARGETS="${1:-all}"

if ! command -v "$CONTAINER_RT" >/dev/null 2>&1; then
    echo "Error: $CONTAINER_RT not found" >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

VERSION=$(awk '/^project\(vermouth VERSION/{print $3}' "$PROJECT_DIR/CMakeLists.txt")

echo -e "${BOLD}Building vermouth $VERSION${RESET}"
echo "Output: $OUTPUT_DIR"

# Build a package in a container, rename it with a distro suffix, copy to OUTPUT_DIR.
# Usage: run_pkg_build <label> <image> <install_cmd> <build_cmd> <glob> <distro_suffix>
run_pkg_build() {
    local label="$1"
    local image="$2"
    local install_cmd="$3"
    local build_cmd="$4"
    local glob="$5"
    local suffix="$6"

    echo -e "\n${BOLD}=== $label ===${RESET}"
    echo "Image: $image"

    if ! "$CONTAINER_RT" pull --quiet "$image" 2>/dev/null; then
        skip "$label (could not pull image)"
        return 1
    fi

    local log rc
    log=$("$CONTAINER_RT" run --rm \
        -v "$PROJECT_DIR":/src:ro,z \
        -v "$OUTPUT_DIR":/out:z \
        "$image" \
        bash -c "
            set -euo pipefail
            $install_cmd
            cp -r /src /build
            cd /build
            $build_cmd
            for f in $glob; do
                [ -f \"\$f\" ] || continue
                ext=\"\${f##*.}\"
                base=\"\${f%.*}\"
                cp \"\$f\" \"/out/\${base}-${suffix}.\${ext}\"
            done
        " 2>&1) && rc=0 || rc=$?

    if [ $rc -ne 0 ]; then
        fail "$label build failed"
        echo "$log" | tail -20 | sed 's/^/    /'
        return 1
    fi
    return 0
}

# Install a package in a fresh container and verify the binary is present.
# Usage: run_install_test <label> <image> <install_cmd> <pkg_glob>
run_install_test() {
    local label="$1"
    local image="$2"
    local install_cmd="$3"
    local pkg_glob="$4"

    local pkg
    pkg=$(find "$OUTPUT_DIR" -maxdepth 1 -name "$pkg_glob" 2>/dev/null | head -1)
    if [ -z "$pkg" ]; then
        fail "$label install test — package not found ($pkg_glob)"
        return
    fi

    local pkg_file
    pkg_file=$(basename "$pkg")

    local log rc
    log=$("$CONTAINER_RT" run --rm \
        -v "$OUTPUT_DIR":/out:ro,z \
        "$image" \
        bash -c "
            set -euo pipefail
            $install_cmd /out/$pkg_file
            test -x \"\$(command -v vermouth)\"
        " 2>&1) && rc=0 || rc=$?

    if [ $rc -eq 0 ]; then
        pass "$label — installs and binary present"
    else
        fail "$label — install test failed"
        echo "$log" | tail -20 | sed 's/^/    /'
    fi
}

# --- Fedora RPM ---
build_rpm_fedora() {
    local built=0
    run_pkg_build "Fedora RPM" "fedora:latest" \
        "dnf install -y --quiet cmake gcc-c++ extra-cmake-modules \
            qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtquickcontrols2-devel \
            kf6-kirigami-devel kf6-kcoreaddons-devel kf6-ki18n-devel \
            kf6-kcolorscheme-devel kf6-kconfig-devel kf6-qqc2-desktop-style icoutils rpm-build SDL2-devel" \
        "cmake -S . -B _build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
         cmake --build _build --parallel \$(nproc)
         cpack --config _build/CPackConfig.cmake -G RPM" \
        "*.rpm" "fedora" && built=1 || true

    if [ $built -eq 1 ]; then
        pass "Fedora RPM — package written to dist/"
        run_install_test "Fedora RPM install" "fedora:latest" \
            "dnf install -y --quiet" \
            "*-fedora.rpm"
    fi
}

# --- OpenSUSE RPM ---
build_rpm_opensuse() {
    local built=0
    run_pkg_build "OpenSUSE RPM" "opensuse/tumbleweed" \
        "zypper --non-interactive refresh && zypper --non-interactive install -y \
            cmake gcc-c++ extra-cmake-modules \
            qt6-base-devel qt6-declarative-devel \
            kf6-kirigami-devel kf6-kcoreaddons-devel kf6-ki18n-devel \
            kf6-kcolorscheme-devel kf6-kconfig-devel kf6-qqc2-desktop-style-devel icoutils rpm-build SDL2-devel" \
        "cmake -S . -B _build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
         cmake --build _build --parallel \$(nproc)
         cpack --config _build/CPackConfig.cmake -G RPM" \
        "*.rpm" "opensuse" && built=1 || true

    if [ $built -eq 1 ]; then
        pass "OpenSUSE RPM — package written to dist/"
        run_install_test "OpenSUSE RPM install" "opensuse/tumbleweed" \
            "zypper --non-interactive refresh && zypper --non-interactive --no-gpg-checks install -y" \
            "*-opensuse.rpm"
    fi
}

# --- Ubuntu DEB ---
build_deb() {
    local built=0
    run_pkg_build "Ubuntu DEB" "ubuntu:25.04" \
        "export DEBIAN_FRONTEND=noninteractive
         apt-get update -qq
         apt-get install -y -qq build-essential cmake extra-cmake-modules dpkg-dev \
            qt6-base-dev qt6-declarative-dev qt6-tools-dev-tools \
            libkirigami-dev libkf6coreaddons-dev libkf6i18n-dev \
            libkf6colorscheme-dev libkf6config-dev libkf6qqc2desktopstyle-dev libsdl2-dev file" \
        "cmake -S . -B _build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
         cmake --build _build --parallel \$(nproc)
         cpack --config _build/CPackConfig.cmake -G DEB" \
        "*.deb" "ubuntu" && built=1 || true

    if [ $built -eq 1 ]; then
        pass "Ubuntu DEB — package written to dist/"
        run_install_test "Ubuntu DEB install" "ubuntu:25.04" \
            "export DEBIAN_FRONTEND=noninteractive && apt-get update -qq && apt-get install -y -qq" \
            "*-ubuntu.deb"
    fi
}

# --- Arch package ---
build_arch() {
    echo -e "\n${BOLD}=== Arch Package ===${RESET}"
    echo "Image: archlinux:latest"

    if ! "$CONTAINER_RT" pull --quiet "archlinux:latest" 2>/dev/null; then
        skip "Arch (could not pull image)"
        return
    fi

    local log rc
    log=$("$CONTAINER_RT" run --rm \
        -v "$PROJECT_DIR":/src:ro,z \
        -v "$OUTPUT_DIR":/out:z \
        archlinux:latest \
        bash -c "
            set -euo pipefail
            pacman -Syu --noconfirm --quiet
            pacman -S --noconfirm --needed --quiet \
                base-devel cmake ninja extra-cmake-modules \
                qt6-base qt6-declarative \
                kirigami ki18n kcoreaddons kcolorscheme kconfig qqc2-desktop-style icoutils sdl2
            useradd -m builder

            PKGVER=\$(grep -oP 'project\(vermouth VERSION \K[0-9.]+' /src/CMakeLists.txt)

            # Copy source, stripping any existing build artifacts that would
            # confuse cmake with stale host paths in CMakeCache.txt
            cp -r /src /tmp/vermouth-\${PKGVER}
            rm -rf /tmp/vermouth-\${PKGVER}/build \
                   /tmp/vermouth-\${PKGVER}/_build \
                   /tmp/vermouth-\${PKGVER}/dist
            tar -czf /tmp/vermouth-\${PKGVER}.tar.gz -C /tmp vermouth-\${PKGVER}
            SHA256=\$(sha256sum /tmp/vermouth-\${PKGVER}.tar.gz | awk '{print \$1}')

            # Set up build directory entirely outside the mounted source
            mkdir -p /home/builder/build
            cp /src/packaging/PKGBUILD /home/builder/build/PKGBUILD
            cp /tmp/vermouth-\${PKGVER}.tar.gz /home/builder/build/

            # Point PKGBUILD at local tarball with computed SHA
            sed -i \"s|^pkgver=.*|pkgver=\${PKGVER}|\" /home/builder/build/PKGBUILD
            sed -i \"s|^source=.*|source=('vermouth-\${PKGVER}.tar.gz')|\" /home/builder/build/PKGBUILD
            sed -i \"s|^sha256sums=.*|sha256sums=('\${SHA256}')|\" /home/builder/build/PKGBUILD

            chown -R builder:builder /home/builder/build
            su builder -c 'cd /home/builder/build && makepkg -sf --noconfirm'
            find /home/builder/build -maxdepth 1 -name '*.pkg.tar.zst' -exec cp {} /out/ \;
        " 2>&1) && rc=0 || rc=$?

    if [ $rc -eq 0 ]; then
        local found
        found=$(find "$OUTPUT_DIR" -maxdepth 1 -name "*.pkg.tar.zst" 2>/dev/null | wc -l)
        if [ "$found" -gt 0 ]; then
            pass "Arch — package written to dist/"
        else
            fail "Arch — build succeeded but no package found"
            return
        fi
    else
        fail "Arch build failed"
        echo "$log" | tail -20 | sed 's/^/    /'
        return
    fi

    # Install test in fresh container
    local pkg
    pkg=$(find "$OUTPUT_DIR" -maxdepth 1 -name "*.pkg.tar.zst" | head -1)
    local pkg_file
    pkg_file=$(basename "$pkg")

    log=$("$CONTAINER_RT" run --rm \
        -v "$OUTPUT_DIR":/out:ro,z \
        archlinux:latest \
        bash -c "
            set -euo pipefail
            pacman -Syu --noconfirm --quiet
            pacman -U --noconfirm /out/$pkg_file
            test -x \"\$(command -v vermouth)\"
        " 2>&1) && rc=0 || rc=$?

    if [ $rc -eq 0 ]; then
        pass "Arch install — installs and binary present"
    else
        fail "Arch install test failed"
        echo "$log" | tail -20 | sed 's/^/    /'
    fi
}

# --- Flatpak ---
build_flatpak() {
    echo -e "\n${BOLD}=== Flatpak ===${RESET}"
    echo "Image: fedora:43"

    if ! "$CONTAINER_RT" pull --quiet "fedora:43" 2>/dev/null; then
        skip "Flatpak (could not pull image)"
        return
    fi

    local log rc
    log=$("$CONTAINER_RT" run --rm --privileged \
        -v "$PROJECT_DIR":/src:ro,z \
        -v "$OUTPUT_DIR":/out:z \
        fedora:43 \
        bash -c "
            set -euo pipefail
            dnf upgrade --refresh -y --quiet
            dnf install -y --quiet flatpak-builder
            flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

            cp -r /src /build
            cd /build
            rm -rf build _build dist

            flatpak-builder --force-clean --repo=flatpakrepo build-dir com.dekomote.vermouth.yml \
                --install-deps-from=flathub --disable-rofiles-fuse
            flatpak build-bundle flatpakrepo vermouth-${VERSION}.flatpak com.dekomote.vermouth
            cp vermouth-*.flatpak /out/
        " 2>&1) && rc=0 || rc=$?

    if [ $rc -eq 0 ] && find "$OUTPUT_DIR" -maxdepth 1 -name "*.flatpak" | grep -q .; then
        pass "Flatpak — package written to dist/"
    else
        fail "Flatpak build failed"
        echo "$log" | tail -20 | sed 's/^/    /'
    fi
}

# --- AppImage ---
APPIMAGE_BUILDER_IMAGE="vermouth-appimage-builder:latest"
APPIMAGE_TOOL_CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/vermouth-appimage-tools"

ensure_appimage_builder() {
    if "$CONTAINER_RT" image exists "$APPIMAGE_BUILDER_IMAGE" 2>/dev/null; then
        return 0
    fi
    echo "  Building cached builder image (one-time)…"
    "$CONTAINER_RT" build --quiet -t "$APPIMAGE_BUILDER_IMAGE" -f - <<'DOCKERFILE'
FROM ubuntu:25.10
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && apt-get install -y -qq \
    build-essential cmake extra-cmake-modules file wget fuse3 \
    qt6-base-dev qt6-declarative-dev qt6-tools-dev-tools qmake6 \
    libkirigami-dev libkf6coreaddons-dev libkf6i18n-dev libkf6colorscheme-dev libkf6config-dev libkf6qqc2desktopstyle-dev \
    qml6-module-org-kde-kirigami qml6-module-org-kde-desktop \
    qml6-module-org-kde-iconthemes \
    qt6-wayland libsdl2-dev qml-module-org-kde-qqc2desktopstyle qt6-xdgdesktopportal-platformtheme \
    libqt6waylandclient6 \
    && rm -rf /var/lib/apt/lists/*
DOCKERFILE
}

build_appimage() {
    echo -e "\n${BOLD}=== AppImage ===${RESET}"

    if ! "$CONTAINER_RT" pull --quiet "ubuntu:25.10" 2>/dev/null; then
        skip "AppImage (could not pull image)"
        return
    fi

    ensure_appimage_builder

    mkdir -p "$APPIMAGE_TOOL_CACHE"

    local log rc
    log=$("$CONTAINER_RT" run --rm \
        -v "$PROJECT_DIR":/src:ro,z \
        -v "$OUTPUT_DIR":/out:z \
        -v "$APPIMAGE_TOOL_CACHE":/toolcache:z \
        "$APPIMAGE_BUILDER_IMAGE" \
        bash -c "
            set -euo pipefail

            # Download linuxdeploy tools only if not already cached
            [ -f /toolcache/linuxdeploy ] || \
                wget -q -O /toolcache/linuxdeploy 'https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage'
            [ -f /toolcache/linuxdeploy-plugin-qt ] || \
                wget -q -O /toolcache/linuxdeploy-plugin-qt 'https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage'

            cp -r /src /build
            cd /build
            rm -rf build _build dist

            cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -Wno-dev
            cmake --build build --parallel \$(nproc)
            DESTDIR=AppDir cmake --install build

            cp /toolcache/linuxdeploy /toolcache/linuxdeploy-plugin-qt .
            chmod +x linuxdeploy linuxdeploy-plugin-qt

            export QML_SOURCES_PATHS=/build/qml
            export QML_MODULES_PATHS=/usr/lib/x86_64-linux-gnu/qt6/qml
            export EXTRA_QT_MODULES='xcbintegration;waylandcompositor;svg;qml;platform'
            export DEPLOY_PLATFORM_THEMES=1
            export APPIMAGE_EXTRACT_AND_RUN=1
            export VERSION=${VERSION}

            ./linuxdeploy --appdir AppDir \
                --desktop-file AppDir/usr/share/applications/com.dekomote.vermouth.desktop \
                --icon-file AppDir/usr/share/icons/hicolor/scalable/apps/com.dekomote.vermouth.svg \
                --plugin qt \
                --output appimage
            cp Vermouth-*.AppImage /out/
        " 2>&1) && rc=0 || rc=$?

    if [ $rc -eq 0 ] && find "$OUTPUT_DIR" -maxdepth 1 -name "*.AppImage" | grep -q .; then
        pass "AppImage — package written to dist/"
    else
        fail "AppImage build failed"
        echo "$log" | tail -20 | sed 's/^/    /'
    fi
}

case "$TARGETS" in
    all)
        build_rpm_fedora
        build_rpm_opensuse
        build_arch
        build_flatpak
        build_appimage
        build_deb
        ;;
    fedora)                  build_rpm_fedora ;;
    opensuse)                build_rpm_opensuse ;;
    deb)                     build_deb ;;
    arch)                    build_arch ;;
    flatpak)                 build_flatpak ;;
    appimage)                build_appimage ;;
    appimage-rebuild-cache)
        "$CONTAINER_RT" rmi "$APPIMAGE_BUILDER_IMAGE" 2>/dev/null || true
        rm -f "$APPIMAGE_TOOL_CACHE"/linuxdeploy "$APPIMAGE_TOOL_CACHE"/linuxdeploy-plugin-qt
        build_appimage
        ;;
    *)
        echo "Usage: $0 [all|fedora|opensuse|deb|arch|flatpak|appimage|appimage-rebuild-cache]"
        exit 1
        ;;
esac

echo -e "\n${BOLD}=== Summary ===${RESET}"
echo -e "${GREEN}Passed: $PASSED${RESET}  ${RED}Failed: $FAILED${RESET}  ${YELLOW}Skipped: $SKIPPED${RESET}"

if [ $PASSED -gt 0 ]; then
    echo -e "\n${BOLD}Packages in $OUTPUT_DIR:${RESET}"
    ls -lh "$OUTPUT_DIR"
fi

[ $FAILED -eq 0 ] || exit 1
