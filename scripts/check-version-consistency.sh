#!/bin/bash
# Pre-commit hook: verify that the version in CMakeLists.txt, the flathub manifest,
# and the current git tag (if any) are consistent.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

CMAKE_VERSION=$(grep -oP 'project\(vermouth VERSION \K[0-9.]+' "${PROJECT_DIR}/CMakeLists.txt")
FLATHUB_TAG=$(grep -oP 'tag:\s*v?\K[0-9.]+' "${PROJECT_DIR}/com.dekomote.vermouth.flathub.yml")
SPEC_VERSION=$(awk '/^Version:/{print $2}' "${PROJECT_DIR}/packaging/vermouth.spec")
PKGBUILD_VERSION=$(awk -F= '/^pkgver=/{print $2}' "${PROJECT_DIR}/packaging/PKGBUILD")
GIT_TAG=$(git describe --tags --exact-match HEAD 2>/dev/null | sed 's/^v//' || true)

ERRORS=0

if [ "$CMAKE_VERSION" != "$FLATHUB_TAG" ]; then
    echo "Version mismatch: CMakeLists.txt=${CMAKE_VERSION} vs flathub manifest tag=${FLATHUB_TAG}"
    ERRORS=1
fi

if [ "$CMAKE_VERSION" != "$SPEC_VERSION" ]; then
    echo "Version mismatch: CMakeLists.txt=${CMAKE_VERSION} vs packaging/vermouth.spec=${SPEC_VERSION}"
    ERRORS=1
fi

if [ "$CMAKE_VERSION" != "$PKGBUILD_VERSION" ]; then
    echo "Version mismatch: CMakeLists.txt=${CMAKE_VERSION} vs packaging/PKGBUILD=${PKGBUILD_VERSION}"
    ERRORS=1
fi

if [ -n "$GIT_TAG" ] && [ "$GIT_TAG" != "$CMAKE_VERSION" ]; then
    echo "Version mismatch: CMakeLists.txt=${CMAKE_VERSION} vs git tag=${GIT_TAG}"
    ERRORS=1
fi

if [ "$ERRORS" -eq 0 ]; then
    echo "Version consistency OK: ${CMAKE_VERSION}"
fi

exit $ERRORS
