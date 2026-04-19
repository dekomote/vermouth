#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PACKAGING_DIR="$PROJECT_DIR/packaging"

CONTAINER_RT="${CONTAINER_RT:-podman}"

if ! command -v "$CONTAINER_RT" >/dev/null 2>&1; then
    echo "Error: $CONTAINER_RT not found" >&2
    exit 1
fi

echo "Updating PKGBUILD sha256sums and generating .SRCINFO..."

"$CONTAINER_RT" run --rm \
    -v "$PACKAGING_DIR":/pkg \
    archlinux:latest \
    bash -c "
        pacman -Syu --noconfirm --quiet
        pacman -S --noconfirm --needed --quiet pacman-contrib
        useradd -m builder
        cp -r /pkg /tmp/pkg
        chown -R builder:builder /tmp/pkg
        su builder -c 'cd /tmp/pkg && updpkgsums && makepkg --printsrcinfo > .SRCINFO'
        cp /tmp/pkg/PKGBUILD /pkg/PKGBUILD
        cp /tmp/pkg/.SRCINFO /pkg/.SRCINFO
    "

echo "Done."
echo "  packaging/PKGBUILD — sha256sums updated"
echo "  packaging/.SRCINFO  — generated"
