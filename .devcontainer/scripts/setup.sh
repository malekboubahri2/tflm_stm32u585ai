#!/bin/bash

export DEBIAN_FRONTEND=noninteractive

#installing dependencies
apt-get update && apt-get install -y --no-install-recommends \
wget \
git \
build-essential \
cmake \
make \
usbutils \
openocd \
python3-pip \
minicom \
&& rm -rf /var/lib/apt/lists/*

#exporting cmake path
export PATH="/opt/$(ls /opt | grep cmake | head -n1)/bin:${PATH}"

# install ARM GCC toolchain (hard‑coded link, always install to
# a stable path so we don't have to include the version in the name)
TOOL_URL="https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz"
TARGET_DIR=/opt/arm-gnu-toolchain
if [ ! -d "$TARGET_DIR" ]; then
    # download into a temp file so we can remove it afterwards
    tmpfile=$(mktemp)
    wget -q "$TOOL_URL" -O "$tmpfile"
    # extract; the archive creates a versioned subdirectory
    tar -xJf "$tmpfile" -C /opt
    rm -f "$tmpfile"
    # find the newly created directory and symlink it to the stable name
    extracted=$(ls -d /opt/arm-gnu-toolchain-* | head -n1)
    if [ -n "$extracted" ]; then
        ln -sfn "$extracted" "$TARGET_DIR"
    fi
fi

# prepend the new toolchain to PATH
export PATH="$TARGET_DIR/bin:${PATH}"
