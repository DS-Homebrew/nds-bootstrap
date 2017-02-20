$DEVKITARM/bin/ndstool -c bootstrap.nds -7 nds-bootstrap.arm7.elf -9 nds-bootstrap.arm9.elf -b icon.bmp "NDS BOOTSTRAP;Runs an .nds file;made by Ahezard" -g KBSE 01 "NDSBOOTSTRAP" -z 80040000 -u 00030004

$DEVKITARM/bin/dlditool dldi/dsisd.dldi bootstrap.nds

python patch_ndsheader_dsiware.py bootstrap.nds

./make_cia --srl=bootstrap.nds