#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build-appimage"
APPDIR="$PROJECT_DIR/AppDir"
TOOLS_DIR="$PROJECT_DIR/.appimage-tools"

echo "=== Building Vermouth AppImage ==="

mkdir -p "$TOOLS_DIR"

APPIMAGETOOL="$TOOLS_DIR/appimagetool-940-x86_64.AppImage"
if [ ! -x "$APPIMAGETOOL" ]; then
    echo "Downloading appimagetool"
    wget -q -O "$APPIMAGETOOL" \
        "https://github.com/probonopd/go-appimage/releases/download/continuous/appimagetool-940-x86_64.AppImage"
    chmod +x "$APPIMAGETOOL"
fi

rm -rf "$BUILD_DIR" "$APPDIR"

# Build
echo "Configuring"
cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -Wno-dev

echo "Building"
cmake --build "$BUILD_DIR" --parallel

echo "Installing into AppDir"
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR"

# Resolve Qt/KDE paths
QMAKE="${QMAKE:-$(which qmake6 2>/dev/null || which qmake 2>/dev/null)}"
if [ -z "$QMAKE" ]; then
    echo "Error: qmake6 or qmake not found. Set QMAKE env variable."
    exit 1
fi
QT_QML_DIR=$("$QMAKE" -query QT_INSTALL_QML)
QT_PLUGIN_DIR=$("$QMAKE" -query QT_INSTALL_PLUGINS)
QT_LIB_DIR=$("$QMAKE" -query QT_INSTALL_LIBS)

# Copy Qt QML modules
echo "Bundling QML modules"
mkdir -p "$APPDIR/usr/qml"
QML_MODULES=(
    QtQuick
    QtQuick/Controls
    QtQuick/Dialogs
    QtQuick/Layouts
    QtQuick/Templates
    QtQuick/Window
    Qt/labs/folderlistmodel
    Qt/labs/platform
)
for mod in "${QML_MODULES[@]}"; do
    if [ -d "$QT_QML_DIR/$mod" ]; then
        mkdir -p "$APPDIR/usr/qml/$(dirname "$mod")"
        cp -a "$QT_QML_DIR/$mod" "$APPDIR/usr/qml/$mod"
    fi
done

# Copy KDE QML modules
echo "Bundling KDE QML modules"
KDE_QML_MODULES=(
    org/kde/kirigami
    org/kde/i18n
    org/kde/coreaddons
    org/kde/desktop
    org/kde/iconthemes
    org/kde/sonnet
    org/kde/qqc2desktopstyle
)
for mod in "${KDE_QML_MODULES[@]}"; do
    if [ -d "$QT_QML_DIR/$mod" ]; then
        mkdir -p "$APPDIR/usr/qml/$(dirname "$mod")"
        cp -a "$QT_QML_DIR/$mod" "$APPDIR/usr/qml/$mod"
    fi
done

# Copy Qt plugins
echo "Bundling Qt plugins"
mkdir -p "$APPDIR/usr/plugins"
QT_PLUGINS=(
    platforms/libqxcb.so
    platforms/libqwayland-generic.so
    platformthemes/libqxdgdesktopportal.so
    platformthemes/libqgtk3.so
    platformthemes/KDEPlasmaPlatformTheme6.so
    imageformats/libqsvg.so
    iconengines/libqsvgicon.so
    styles/breeze6.so
    xcbglintegrations
)
for plug in "${QT_PLUGINS[@]}"; do
    if [ -e "$QT_PLUGIN_DIR/$plug" ]; then
        mkdir -p "$APPDIR/usr/plugins/$(dirname "$plug")"
        cp -a "$QT_PLUGIN_DIR/$plug" "$APPDIR/usr/plugins/$plug"
    fi
done

# Bundle shared libraries and their dependencies
echo "Bundling shared libraries"
mkdir -p "$APPDIR/usr/lib"

bundle_lib_deps() {
    { ldd "$1" 2>/dev/null | grep "=> /" | awk '{print $3}' || true; } | while read -r lib; do
        local base
        base=$(basename "$lib")
        # Skip glibc and low-level system libs that must come from the host
        case "$base" in
            libc.so*|libm.so*|libdl.so*|librt.so*|libpthread.so*|libgcc_s.so*|\
            libstdc++.so*|ld-linux*|libGL.so*|libGLX.so*|libGLdispatch.so*|\
            libEGL.so*|libvulkan.so*|libX11.so*|libX11-xcb.so*|libxcb.so*|\
            libxcb-*.so*|libXext.so*|libXi.so*|libXrender.so*|libXrandr.so*|\
            libdrm.so*|libnvidia*|libwayland-client.so*|libwayland-cursor.so*|\
            libwayland-egl.so*|libwayland-server.so*|\
            libcrypt.so*|libcrypto.so*|libssl.so*|\
            libblkid.so*|libmount.so*|libselinux.so*|libsystemd.so*|\
            libdbus-1.so*|libgio-2.0.so*|libglib-2.0.so*|libgobject-2.0.so*|\
            libgmodule-2.0.so*|libffi.so*|libtinfo.so*|libcap.so*|\
            libfontconfig.so*|libfreetype.so*|libz.so*|libbz2.so*|\
            libpng*.so*|libjpeg.so*|libharfbuzz.so*)
                continue
                ;;
        esac
        if [ ! -e "$APPDIR/usr/lib/$base" ]; then
            cp -L "$lib" "$APPDIR/usr/lib/$base"
        fi
    done
}

# Bundle deps for the main binary
bundle_lib_deps "$APPDIR/usr/bin/vermouth"

# Bundle deps for all .so files we copied (two passes for transitive deps)
for pass in 1 2; do
    find "$APPDIR/usr" -name "*.so*" -type f | while read -r so; do
        bundle_lib_deps "$so"
    done
done

# qt.conf
cat > "$APPDIR/usr/bin/qt.conf" <<'QTEOF'
[Paths]
Prefix = ..
Plugins = plugins
Qml2Imports = qml
QTEOF

# AppRun
cat > "$APPDIR/AppRun" <<'APPRUN'
#!/bin/bash
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export LD_LIBRARY_PATH="$HERE/usr/lib:${LD_LIBRARY_PATH}"
HOST_QT_PLUGIN_PATH="${QT_PLUGIN_PATH:-/usr/lib64/qt6/plugins:/usr/lib/qt6/plugins}"
export QT_PLUGIN_PATH="$HERE/usr/plugins:${HOST_QT_PLUGIN_PATH}"
export QML2_IMPORT_PATH="$HERE/usr/qml"
export QML_IMPORT_PATH="$HERE/usr/qml"
exec "$HERE/usr/bin/vermouth" "$@"
APPRUN
chmod +x "$APPDIR/AppRun"

# Desktop integration symlinks
ln -sf usr/share/applications/vermouth.desktop "$APPDIR/vermouth.desktop"
ln -sf usr/share/icons/hicolor/scalable/apps/vermouth.svg "$APPDIR/vermouth.svg"

export VERSION=$(grep 'project(vermouth' "$PROJECT_DIR/CMakeLists.txt" | grep -oP 'VERSION \K[0-9.]+')

echo "Packaging AppImage"
"$APPIMAGETOOL" "$APPDIR"

mv Vermouth-*.AppImage "$PROJECT_DIR/" 2>/dev/null || true

echo "Done"
ls -lh "$PROJECT_DIR"/Vermouth-*.AppImage
