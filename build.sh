#!/usr/bin/env bash
# Compila o proxy dinput.dll (i686, MinGW-w64) -> build/dinput.dll
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
# testes das contas puras (gcc do Linux) antes da DLL
gcc -std=c11 -O2 -Wall -Wextra -Wredundant-decls -o build/test_hud_math tests/test_hud_math.c src/hud_math.c -lm
build/test_hud_math
gcc -std=c11 -O2 -Wall -Wextra -Wredundant-decls -o build/test_pad_cursor tests/test_pad_cursor.c src/pad_cursor.c
build/test_pad_cursor
gcc -std=c11 -O2 -Wall -Wextra -Wredundant-decls -o build/test_pad_icons tests/test_pad_icons.c src/pad_icons.c
build/test_pad_icons
gcc -std=c11 -O2 -Wall -Wextra -Wredundant-decls -o build/test_menu_center tests/test_menu_center.c src/menu_center.c
build/test_menu_center
gcc -std=c11 -O2 -Wall -Wextra -Wredundant-decls -o build/test_stick_shape tests/test_stick_shape.c src/stick_shape.c -lm
build/test_stick_shape
gcc -std=c11 -O2 -Wall -Wextra -Wredundant-decls -o build/test_glrec tests/test_glrec.c src/glrec.c
build/test_glrec
gcc -std=c11 -O2 -Wall -Wextra -Wredundant-decls -o build/test_pacer tests/test_pacer.c src/pacer.c
build/test_pacer
gcc -std=c11 -O2 -Wall -Wextra -Wredundant-decls -o build/test_interp tests/test_interp.c src/interp.c -lm
build/test_interp
gcc -std=c11 -O2 -Wall -Wextra -Wredundant-decls -o build/test_coloradj tests/test_coloradj.c src/coloradj.c -lm
build/test_coloradj
i686-w64-mingw32-gcc -std=c11 -O2 -msse2 -mfpmath=sse -Wall -Wextra -Wredundant-decls -Wno-unused-parameter -Wno-cast-function-type -Wno-misleading-indentation \
    -shared -static-libgcc -Wl,--enable-stdcall-fixup \
    -o build/dinput.dll src/bbfix.c src/hud_math.c src/pad_cursor.c src/pad_icons.c src/menu_center.c src/stick_shape.c src/glrec.c src/pacer.c src/interp.c src/coloradj.c src/dinput.def -lopengl32 -lgdi32 -luser32 -ladvapi32
i686-w64-mingw32-strip build/dinput.dll
cp dist/bblit2fix.ini build/
ls -la build/
