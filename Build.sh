#!/bin/sh

#  Build.sh
#  LeapMotion
#
#  Created by Sam Marshall on 11/15/13.
#  Copyright (c) 2013 Sam Marshall. All rights reserved.

KEXT_NAME="LeapMotion"

sudo kextunload -v /System/Library/Extensions/LeapMotion.kext

if [ "${ACTION}" = "clean" ]; then
cd "${PROJECT_DIR}"
xcodebuild -target "$KEXT_NAME" -configuration ${CONFIGURATION} -sdk macosx clean
fi

if [ "${ACTION}" = "build" ]; then
cd "${PROJECT_DIR}"
xcodebuild -target "$KEXT_NAME" -configuration ${CONFIGURATION} -sdk macosx


cd "${PROJECT_DIR}/build/${CONFIGURATION}/"
sudo cp -R LeapMotion.kext /System/Library/Extensions/
sudo kextload -v /System/Library/Extensions/LeapMotion.kext
fi