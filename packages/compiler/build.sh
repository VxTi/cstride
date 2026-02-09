#!/usr/bin/env bash

cmake -S . -B cmake-build-debug -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/llvm && cmake --build cmake-build-debug