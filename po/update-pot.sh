#!/bin/sh
# Extracts translatable strings from C++ and QML sources into vermouth.pot

BASEDIR=$(dirname "$0")/..
POTFILE="$BASEDIR/po/vermouth.pot"

find "$BASEDIR/src" -name "*.cpp" -o -name "*.h" | sort | \
xgettext --files-from=- \
    --language=C++ \
    --keyword=i18n \
    --keyword=i18nc:1c,2 \
    --keyword=i18np:1,2 \
    --keyword=i18ncp:1c,2,3 \
    --from-code=UTF-8 \
    --output="$POTFILE" \
    --package-name=vermouth \

find "$BASEDIR/qml" -name "*.qml" | sort | \
xgettext --files-from=- \
    --language=JavaScript \
    --keyword=i18n \
    --keyword=i18nc:1c,2 \
    --keyword=i18np:1,2 \
    --keyword=i18ncp:1c,2,3 \
    --from-code=UTF-8 \
    --join-existing \
    --output="$POTFILE" \
    --package-name=vermouth \

echo "Updated $POTFILE"
