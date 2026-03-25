#!/bin/bash

set -euo pipefail

wheel="$1"
basename="$(basename "$wheel")"

for bucket in everlaw-build-artifacts everlaw-artifacts; do
    url="s3://$bucket/fasttext/$basename"
    if aws s3 ls "$url" &>/dev/null; then
        >&2 echo "$url already exists!"
    else
        aws s3 cp --only-show-errors "$wheel" "$url"
        >&2 echo "Uploaded $url"
    fi
done
