#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

#---------------------------------------------------------------------------------
# TARGET is the name of the output
#---------------------------------------------------------------------------------
export TARGET	:=	nds-bootstrap
export TOPDIR	:=	$(CURDIR)
BIN				:=	bin
ASSETS			:=	assets
GRAPHICS		:=  $(ASSETS)/gfx

# specify a directory which contains the nitro filesystem
# this is relative to the Makefile
NITRO_FILES	:=

# These set the information text in the nds file
GAME_TITLE		:= NDS BOOTSTRAP
GAME_SUBTITLE1	:= Runs an .nds file
GAME_SUBTITLE2	:= Made by Ahezard

VERSION_MAJOR	:= 0
VERSION_MINOR	:= 11
VERSION_PATCH	:= 1

include $(DEVKITARM)/ds_rules


VERSION	:=	$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

export GAME_ICON := $(CURDIR)/$(ASSETS)/icon.bmp

export CC		:=	$(PREFIX)gcc
export CXX		:=	$(PREFIX)g++
export AR		:=	$(PREFIX)ar
export OBJCOPY	:=	$(PREFIX)objcopy
export CPP		:=	$(PREFIX)cpp

.PHONY: all dist release nightly bootloader cardengine_arm7 cardengine_arm9 clean

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all:	$(CURDIR)/$(BIN)/$(TARGET).nds

#---------------------------------------------------------------------------------
dist:	all
#	@rm	-fr	hbmenu
#	@mkdir hbmenu
#	@cp hbmenu.nds hbmenu/BOOT.NDS
#	@cp BootStrap/_BOOT_MP.NDS BootStrap/TTMENU.DAT BootStrap/_DS_MENU.DAT BootStrap/ez5sys.bin BootStrap/akmenu4.nds hbmenu
#	@tar -cvjf hbmenu-$(VERSION).tar.bz2 hbmenu testfiles README.md COPYING -X exclude.lst

#---------------------------------------------------------------------------------
release:	$(CURDIR)/$(BIN)/$(TARGET).nds
	@rm -f $(CURDIR)/$(BIN)/$(TARGET)-release.nds
	@mv $(CURDIR)/$(BIN)/$(TARGET).nds $(CURDIR)/$(BIN)/$(TARGET)-release.nds

#---------------------------------------------------------------------------------
nightly:	$(CURDIR)/$(BIN)/$(TARGET).nds
	@rm -f $(CURDIR)/$(BIN)/$(TARGET)-nightly.nds
	@mv $(CURDIR)/$(BIN)/$(TARGET).nds $(CURDIR)/$(BIN)/$(TARGET)-nightly.nds

#---------------------------------------------------------------------------------
#$(BUILD):
#	@[ -d $@ ] || mkdir -p $@
#	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
$(DATA):
	@mkdir -p $(DATA)

#---------------------------------------------------------------------------------
$(BIN):
	@mkdir -p $(BIN)

#---------------------------------------------------------------------------------
$(CURDIR)/$(BIN)/$(TARGET).nds:	$(BIN) $(NITRO_FILES) arm7/$(TARGET).elf arm9/$(TARGET).elf
	ndstool	-c $(CURDIR)/$(BIN)/$(TARGET).nds -7 arm7/$(TARGET).elf -9 arm9/$(TARGET).elf \
			-b $(GAME_ICON) "$(GAME_TITLE);$(GAME_SUBTITLE1);$(GAME_SUBTITLE2)" \
			-g KBSE 01 "NDSBOOTSTRAP" -z 80040000 -u 00030004 -a 00000138 -p 00000001

#---------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	@$(MAKE) -C arm7
	
#---------------------------------------------------------------------------------
arm9/$(TARGET).elf:	bootloader
	@$(MAKE) -C arm9

#---------------------------------------------------------------------------------		
bootloader: $(DATA) cardengine_arm7 cardengine_arm9
	@$(MAKE) -C bootloader

#---------------------------------------------------------------------------------		
cardengine_arm7: $(DATA)
	@$(MAKE) -C cardengine/arm7

#---------------------------------------------------------------------------------		
cardengine_arm9: $(DATA)
	@$(MAKE) -C cardengine/arm9

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(DATA) $(BIN) $(TARGET).elf $(TARGET).nds
	@$(MAKE) -C arm7 clean
	@$(MAKE) -C arm9 clean
	@$(MAKE) -C cardengine/arm7 clean
	@$(MAKE) -C cardengine/arm9 clean
	@$(MAKE) -C bootloader clean
