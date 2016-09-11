GAME_TITLE="NDS BOOTSTRAP"
GAME_SUBTITLE1="Runs an .ds file"
GAME_SUBTITLE2="made by devkitpro"
GAME_INFO="KBSE 01 NTRBOOSTRAP"

$DEVKITARM/bin/ndstool	-c bootstrap.nds -7 nds-bootstrap.arm7.elf -9 nds-bootstrap.arm9.elf -g $GAME_INFO -b icon.bmp  "$GAME_TITLE;$GAME_SUBTITLE1;$GAME_SUBTITLE2"  -r9 0x2000000 -r7 0x2380000 -e9 0x2000000 -e7 0x2380000
python patch_ndsheader_dsiware.py --mode dsi bootstrap.nds

./make_cia.exe --srl=bootstrap.nds