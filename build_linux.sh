#!/bin/sh
# Compile VELOCITY pour Linux (X11/XWayland + OpenGL + ALSA) et assemble Velocity-Linux/ + Velocity-Linux.tar.gz.
# Paquets de developpement (Debian / Ubuntu) :
#   sudo apt install gcc libx11-dev libgl-dev libasound2-dev libfreetype-dev
# Fedora : gcc libX11-devel mesa-libGL-devel alsa-lib-devel freetype-devel
# Arch   : gcc libx11 mesa alsa-lib freetype2
# Puis : ./build_linux.sh && ./Velocity-Linux/velocity
set -e
cd "$(dirname "$0")"

CC=${CC:-gcc}
FT_CFLAGS=$(pkg-config --cflags freetype2 2>/dev/null || echo "-I/usr/include/freetype2")
FT_LIBS=$(pkg-config --libs freetype2 2>/dev/null || echo "-lfreetype")

OUT=Velocity-Linux
rm -rf "$OUT"
mkdir -p "$OUT"

# -Wno-format-truncation : gcc signale chaque snprintf qui pourrait couper (voulu : c'est son role)
# VELOCITY_BUILD : numero de build donne par la CI de release (bouton MISE A JOUR) ; absent = build 0 (dev)
$CC -O2 -std=gnu11 -Wall -Wno-format-truncation ${VELOCITY_BUILD:+-DVELOCITY_BUILD=$VELOCITY_BUILD} $FT_CFLAGS \
    main.c src/plateforme/linux/linux_platform.c \
    -lX11 -lGL -lasound $FT_LIBS -ldl -lpthread -lm \
    -o "$OUT/velocity"

# reglages graphiques modifiables (graphismes/rendu.ini, textures .bmp) : a cote de l'executable
cp -R graphismes "$OUT/graphismes"

tar -czf Velocity-Linux.tar.gz "$OUT"
echo "OK : $OUT/velocity et Velocity-Linux.tar.gz"
