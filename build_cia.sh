GAME_TITLE="NDS BOOTSTRAP"
GAME_SUBTITLE1="Runs an .nds file"
GAME_SUBTITLE2="made by devkitpro"
GAME_INFO="KBSE 01 NDSBOOTSTRAP"

$DEVKITARM/bin/ndstool -c bootstrap.nds -7 nds-bootstrap.arm7.elf -9 nds-bootstrap.arm9.elf -g $GAME_INFO -b icon.bmp  "$GAME_TITLE;$GAME_SUBTITLE1;$GAME_SUBTITLE2"  -r9 0x2000000 -r7 0x2380000 -e9 0x2000000 -e7 0x2380000
cp bootstrap.nds bootstrap-nogba.nds

$DEVKITARM/bin/dlditool dldi/dsisd.dldi bootstrap.nds
$DEVKITARM/bin/dlditool dldi/dsisd.dldi bootstrap-nogba.nds

python patch_ndsheader_dsiware.py --mode dsi bootstrap.nds
python patch_ndsheader_dsiware.py --mode dsinogba bootstrap-nogba.nds

#./make_cia.exe --srl=bootstrap.nds
#./make_cia.exe --srl=bootstrap-dldi.nds