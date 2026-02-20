#!/bin/bash

export DEBIAN_FRONTEND=noninteractive

#installing dependencies
apt-get update
apt-get install -y --no-install-recommends \
git \
build-essential \
cmake \
gcc-arm-none-eabi \
make \
openocd \
python3-pip \

rm -rf /var/lib/apt/lists/*

#exporting cmake path
export PATH="/opt/$(ls /opt | grep cmake | head -n1)/bin:${PATH}"