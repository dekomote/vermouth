#!/bin/bash

VERSION=$(grep -oP 'project\(vermouth VERSION \K[0-9.]+' CMakeLists.txt)
APP_ID=$(grep -oP 'set\(APP_ID "\K[^"]+' CMakeLists.txt)
flatpak-builder --repo=flatpakrepo --force-clean flatpakbuild ${APP_ID}.yml
flatpak build-bundle flatpakrepo vermouth-${VERSION}.flatpak ${APP_ID}
