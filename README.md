# nds-bootstrap
Boot an nds file

nds-bootstrap introduce a dldi driver and an arm7 patcher allowing to have some dldi compatibility without flashcard and without recompilation of homebrews. 

The dldi system is formed of 4 parts :
- a bootloader : this one was taken from the original hbmenu, it loads the homebrew and all the other pieces in memory and transfert the execution to the homebrew.
- a dldi driver : this is the easy part, it detect if the execution take place in arm7 or arm9. If it takes place in arm7 it just access the sdcard. If it takes place in arm9 it writes a command for arm7 in the main memory shared between both, try to trigger some interrupts ar arm7 level then wait for arm7. 
- an arm7 patcher : this is integrated in the bootloader, it modify some part of the homebrew binary (the current method target "interruptDispatcher.s" of libnds) in order to get a "parralel" exection flow to the homebrew. This can be done via the interrupt mechanism and was inspired by NitroHax code (even if the NitroHax original method only work with retail games). This is the smallest part but it is quite hard to implements and debug. Most compatibility improvement in the future should come from this part.
- a arm7 "sdengine" binary : this part is like "server" that waits command from the dldi driver (the "client"), process them (read the sd) then reply to arm9 (put the sd piece of data needed in the main memory and put some special value in memory to notify arm9 that the work is done).

The compatibility is not yet perfect. Here is the compatibility list : 
https://docs.google.com/spreadsheets/d/1M7MxYQzVhb4604esdvo57a7crSvbGzFIdotLW0bm0Co/edit?usp=sharing

It can be configured via the file _nds/nds-bootstrap.ini

I recommand to use a frontend menu to avoid manual ini file modification :
- TWLoader (https://github.com/Robz8/TWLoader)
- nds-hb-menu (https://github.com/ahezard/nds-hb-menu/releases)
