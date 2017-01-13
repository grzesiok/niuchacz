#!/bin/bash

function quit {
    echo "$1"
    exit 1
}

PARAM_ARCH=

if [[ $# -lt 1 ]]; then
    quit "Usage: $0 64|32"
else
    if [[ $1 -eq 32 || $1 -eq 64 ]]; then
        PARAM_ARCH=$1
    else
        quit "Usupported architecture!"
    fi
fi
echo "Building application for arch=$PARAM_ARCH..."
cd src
make ARCH=$PARAM_ARCH build