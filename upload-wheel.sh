#!/bin/bash

set -euo pipefail

wheel="$1"
basename="$(basename "$wheel")"

cosign sign-blob --key awskms:///arn:aws:kms:us-east-1:894611973686:alias/artifact-signing --signing-config /usr/local/etc/signing-config.json --bundle "$wheel.sig" $wheel

for bucket in everlaw-build-artifacts everlaw-artifacts; do
    url="s3://$bucket/fasttext/$basename"
    sig_url="s3://$bucket/fasttext/$basename.sig"
    if aws s3 ls "$url" &>/dev/null; then
        >&2 echo "$url already exists!"
    else
        aws s3 cp --only-show-errors "$wheel.sig" "$sig_url"
        aws s3 cp --only-show-errors "$wheel" "$url"
        >&2 echo "Uploaded $url"
    fi
done
