#!/bin/bash

# Making a memory game I actually want to play everyday

It's hosted [here](https://memory-game-iota-hazel.vercel.app/)

## To Build

1. Ensure you have the Emscripten SDK (emsdk) activated in your terminal.
source ./emsdk/emsdk_env.sh

2. Ensure you have Raylib compiled for WebAssembly.
The easiest way is to use the upstream emscripten port if available,
OR point to your local libraylib.a compiled for web.
Below is a standard command assuming you have 'libraylib.a'
compiled for web in a 'lib/' folder, and headers in 'include/'.
IF you don't have raylib pre-compiled, you can use the -s USE_GLFW=3
flag which often pulls in the necessary windowing code, but you still
usually need the raylib source.

3. Run this below command to compile the .wasm and index.html

em++ -o index.html main.cpp -Os -Wall -I /mnt/c/Users/jk/Documents/memory/emsdk/upstream/emscripten/cache/sysroot/include \
-L /mnt/c/Users/jk/Documents/memory/emsdk/upstream/emscripten/cache/sysroot/lib/libraylib.a -s USE_GLFW=3 -s ASYNCIFY \
--shell-file minshell.html -DPLATFORM_WEB /mnt/c/Users/jk/Documents/memory/emsdk/upstream/emscripten/cache/sysroot/lib/libraylib.a

emrun index.html

Enter this link into your browser: http://172.27.158.184:6931/index.html
