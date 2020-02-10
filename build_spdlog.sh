#!/bin/bash

PLATFORM_ARCH="x86"

if [ "${PLATFORM_ARCH}" == "x86" ]; then
  cd spdlog &&
  mkdir build_x86 && cd build_x86
  cmake ..
  make -j8
fi