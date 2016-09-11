GAME_TITLE="NDS HB MENU"
GAME_SUBTITLE1="Start in DSI then switch to DS mode"
GAME_SUBTITLE2="made by devkitpro"
GAME_INFO="KHNE 01 NTRHOMEBREW"

cp bootstrap/bootstrap.*.elf .
$DEVKITARM/bin/ndstool	-c bootstrap.nds -7 bootstrap.arm7.elf -9 bootstrap.arm9.elf -g $GAME_INFO -b icon.bmp  "$GAME_TITLE;$GAME_SUBTITLE1;$GAME_SUBTITLE2"  -r9 0x2000000 -r7 0x2380000 -e9 0x2000000 -e7 0x2380000
python patch_ndsheader_dsiware.py --read bootstrap.nds > bootstrap.nds_before_patch_header.txt
python patch_ndsheader_dsiware.py --mode dsi bootstrap.nds
python patch_ndsheader_dsiware.py --read bootstrap.nds > bootstrap.nds_after_patch_header.txt

$DEVKITARM/bin/ndstool -c nds-hb-menu.nds -7 hbmenu.arm7.elf -9 hbmenu.arm9.elf -g $GAME_INFO -b icon.bmp  "$GAME_TITLE;$GAME_SUBTITLE1;$GAME_SUBTITLE2"
cp nds-hb-menu.nds hbmenu_ntr.nds
cp nds-hb-menu.nds hbmenu_ntr_r4.nds
cp nds-hb-menu.nds hbmenu_ntr_nogba.nds

./dlditool.exe BootStrap/r4tfv2.dldi nds-hb-menu-dsi-r4.nds

python patch_ndsheader_dsiware.py --mode dsi hbmenu_ntr.nds
python patch_ndsheader_dsiware.py --mode dsi hbmenu_ntr_r4.nds
python patch_ndsheader_dsiware.py --mode dsinogba hbmenu_ntr_nogba.nds

./make_cia.exe --srl=hbmenu_ntr.nds
./make_cia.exe --srl=hbmenu_ntr_r4.nds
./make_cia.exe --srl=bootstrap.nds