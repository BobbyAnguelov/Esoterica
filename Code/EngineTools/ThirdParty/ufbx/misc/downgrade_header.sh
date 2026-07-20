#!/usr/bin/env bash

set -Eeuo pipefail

TAG=$( set -x; python3 misc/get_header_tag.py )

echo "Header version tag: $TAG"
if [[ "$TAG" == v*.*.0 ]]; then
    echo "No older header available"
elif [[ "$TAG" == v*.*.* ]]; then
    OLD_TAG="${TAG%.*}".0
    OLD_COMMIT=$( set -x; git rev-list -n 1 "$OLD_TAG" )
    echo "Downgrading ufbx.h to tag $OLD_TAG, commit $OLD_COMMIT"
    ( set -x; git checkout "$OLD_TAG" -- ufbx.h )
else
    echo "Error: Malformed tag \"$TAG\""
    exit 1
fi
