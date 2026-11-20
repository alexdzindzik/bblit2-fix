#!/usr/bin/env bash
# Compila o proxy dinput.dll (i686, MinGW-w64) -> build/dinput.dll
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
i686-w64-mingw32-gcc -std=c11 -O2 -msse2 -mfpmath=sse -Wall -Wextra -Wredundant-decls -Wno-unused-parameter -Wno-cast-function-type -Wno-misleading-indentation \
    -shared -static-libgcc -Wl,--enable-stdcall-fixup \
    -o build/dinput.dll src/bbfix.c src/dinput.def -lopengl32 -lgdi32 -luser32 -ladvapi32
i686-w64-mingw32-strip build/dinput.dll
cp dist/bblit2fix.ini build/
ls -la build/
