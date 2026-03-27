#!/bin/bash

VERSION=$(git describe --tags --abbrev=0)
flatpak-builder --repo=flatpakrepo --force-clean flatpakbuild com.dekomote.vermouth.yml
flatpak build-bundle flatpakrepo vermouth-${VERSION}.flatpak com.dekomote.vermouth
