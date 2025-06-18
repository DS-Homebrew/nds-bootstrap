#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
.SECONDARY:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

# These set the information text in the nds file
GAME_TITLE     := nds-bootstrap
GAME_SUBTITLE1 := ahezard & Rocket Robz

include $(DEVKITARM)/ds_rules

export VERSION_MAJOR	:= 0
export VERSION_MINOR	:= 17
export VERSION_PATCH	:= 0


VERSION	:=	$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
# DATA is a list of directories containing binary files embedded using bin2o
# GRAPHICS is a list of directories containing image files to be converted with grit
#---------------------------------------------------------------------------------
TARGET		:=	nds-bootstrap
BIN		:=	bin
ASSETS		:=	assets
NITRODATA	:=	nitrofiles

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(BIN)/$(TARGET).nds

export CC		:=	$(PREFIX)gcc
export CXX		:=	$(PREFIX)g++
export AR		:=	$(PREFIX)ar
export OBJCOPY	:=	$(PREFIX)objcopy
export CPP		:=	$(PREFIX)cpp

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

#icons := $(wildcard *.bmp)
#ifneq (,$(findstring $(TARGET).bmp,$(icons)))
#	export GAME_ICON := $(CURDIR)/$(TARGET).bmp
#else
#	ifneq (,$(findstring icon.bmp,$(icons)))
#		export GAME_ICON := $(CURDIR)/icon.bmp
#	endif
#endif
export GAME_ICON := $(CURDIR)/$(ASSETS)/icon.bmp

#.PHONY: cardengine_arm7 cardengine_arm9 bootloader BootStrap clean
.PHONY: all dist release nightly bootloader bootloader2 bootloaderi bootloaderi2 cardengine_arm7 cardengine_arm7_music cardengine_arm9_igm cardengine_arm9_igm_extmem cardengine_arm9 cardengine_arm9_32 cardengine_arm9_start cardengine_arm9_start_foto cardengine_arm9_alt cardengine_arm9_alt_gsdd cardengine_arm9_alt2 cardengine_arm9_alt3 cardengine_arm9_extmem cardengine_arm9_extmem_gsdd cardengine_arm9_extmem_start cardengine_arm9_extmem_foto cardenginei_arm7 cardenginei_arm7_alt cardenginei_arm7_twlsdk cardenginei_arm7_twlsdk3 cardenginei_arm7_dsiware cardenginei_arm7_dsiware3 cardenginei_arm7_cheat cardenginei_arm9_colorlut cardenginei_arm9_igm cardenginei_arm9 cardenginei_arm9_gsdd cardenginei_arm9_dldi cardenginei_arm9_gsdd_dldi cardenginei_arm9_twlsdk cardenginei_arm9_twlsdk3 cardenginei_arm9_twlsdk_dldi cardenginei_arm9_twlsdk3_dldi cardenginei_arm9_dsiware cardenginei_arm9_dsiware3 preLoadSettings apfix mobiclipOffsets dsi2dsSavePatches arm7/$(TARGET).elf arm9/$(TARGET).elf clean

all:	$(OUTPUT)

dist:	all
#	@rm	-fr	hbmenu
#	@mkdir hbmenu
#	@cp hbmenu.nds hbmenu/BOOT.NDS
#	@cp BootStrap/_BOOT_MP.NDS BootStrap/TTMENU.DAT BootStrap/_DS_MENU.DAT BootStrap/ez5sys.bin BootStrap/akmenu4.nds hbmenu
#	@tar -cvjf hbmenu-$(VERSION).tar.bz2 hbmenu testfiles README.md COPYING -X exclude.lst

release:	$(OUTPUT)
	@rm -f $(CURDIR)/$(BIN)/$(TARGET)-release.nds
	@mv $(OUTPUT) $(CURDIR)/$(BIN)/$(TARGET)-release.nds

nightly:	$(OUTPUT)
	@rm -f $(CURDIR)/$(BIN)/$(TARGET)-nightly.nds
	@mv $(OUTPUT) $(CURDIR)/$(BIN)/$(TARGET)-nightly.nds

$(OUTPUT):	$(BIN) arm7/$(TARGET).elf arm9/$(TARGET).elf
	ndstool	-c $(OUTPUT) -7 arm7/$(TARGET).elf -9 arm9/$(TARGET).elf -d $(NITRODATA) \
			-b $(GAME_ICON) "$(GAME_TITLE);$(GAME_SUBTITLE1)" \
			-g KBSE 01 "NDSBOOTSTRAP" -z 80040000 -u 00030004 -a 00000138 -p 00000001

#---------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	@$(MAKE) -C arm7
	
#---------------------------------------------------------------------------------
arm9/$(TARGET).elf:	bootloader bootloader2 bootloaderi bootloaderi2 preLoadSettings apfix mobiclipOffsets dsi2dsSavePatches
	@$(MAKE) -C arm9

#---------------------------------------------------------------------------------		
bootloader: cardengine_arm7 cardengine_arm7_music cardengine_arm9_igm cardengine_arm9_igm_extmem cardengine_arm9 cardengine_arm9_32 cardengine_arm9_start cardengine_arm9_start_foto cardengine_arm9_alt cardengine_arm9_alt_gsdd cardengine_arm9_alt2 cardengine_arm9_alt3 cardengine_arm9_extmem cardengine_arm9_extmem_gsdd cardengine_arm9_extmem_start cardengine_arm9_extmem_foto
	@$(MAKE) -C bootloader

#---------------------------------------------------------------------------------		
bootloader2:
	@$(MAKE) -C bootloader2

#---------------------------------------------------------------------------------		
bootloaderi: cardenginei_arm7 cardenginei_arm7_alt cardenginei_arm7_twlsdk cardenginei_arm7_twlsdk3 cardenginei_arm7_dsiware cardenginei_arm7_dsiware3 cardenginei_arm7_cheat cardenginei_arm9_colorlut cardenginei_arm9_igm cardenginei_arm9 cardenginei_arm9_gsdd cardenginei_arm9_dldi cardenginei_arm9_gsdd_dldi cardenginei_arm9_twlsdk cardenginei_arm9_twlsdk3 cardenginei_arm9_twlsdk_dldi cardenginei_arm9_twlsdk3_dldi cardenginei_arm9_dsiware cardenginei_arm9_dsiware3
	@$(MAKE) -C bootloaderi

#---------------------------------------------------------------------------------		
bootloaderi2:
	@$(MAKE) -C bootloaderi2

#---------------------------------------------------------------------------------		
cardengine_arm7:
	@$(MAKE) -C cardengine/arm7

#---------------------------------------------------------------------------------		
cardengine_arm7_music:
	@$(MAKE) -C cardengine/arm7_music

#---------------------------------------------------------------------------------		
cardengine_arm9_igm:
	@$(MAKE) -C cardengine/arm9_igm

#---------------------------------------------------------------------------------		
cardengine_arm9_igm_extmem:
	@$(MAKE) -C cardengine/arm9_igm_extmem

#---------------------------------------------------------------------------------		
cardengine_arm9:
	@$(MAKE) -C cardengine/arm9

#---------------------------------------------------------------------------------		
cardengine_arm9_32:
	@$(MAKE) -C cardengine/arm9_32

#---------------------------------------------------------------------------------		
cardengine_arm9_start:
	@$(MAKE) -C cardengine/arm9_start

#---------------------------------------------------------------------------------		
cardengine_arm9_start_foto:
	@$(MAKE) -C cardengine/arm9_start_foto

#---------------------------------------------------------------------------------		
cardengine_arm9_alt:
	@$(MAKE) -C cardengine/arm9_alt

#---------------------------------------------------------------------------------		
cardengine_arm9_alt_gsdd:
	@$(MAKE) -C cardengine/arm9_alt_gsdd

#---------------------------------------------------------------------------------		
cardengine_arm9_alt2:
	@$(MAKE) -C cardengine/arm9_alt2

#---------------------------------------------------------------------------------		
cardengine_arm9_alt3:
	@$(MAKE) -C cardengine/arm9_alt3

#---------------------------------------------------------------------------------		
cardengine_arm9_extmem:
	@$(MAKE) -C cardengine/arm9_extmem

#---------------------------------------------------------------------------------		
cardengine_arm9_extmem_gsdd:
	@$(MAKE) -C cardengine/arm9_extmem_gsdd

#---------------------------------------------------------------------------------		
cardengine_arm9_extmem_start:
	@$(MAKE) -C cardengine/arm9_extmem_start

#---------------------------------------------------------------------------------		
cardengine_arm9_extmem_foto:
	@$(MAKE) -C cardengine/arm9_extmem_foto

#---------------------------------------------------------------------------------		
cardenginei_arm7:
	@$(MAKE) -C cardenginei/arm7

#---------------------------------------------------------------------------------		
cardenginei_arm7_alt:
	@$(MAKE) -C cardenginei/arm7_alt

#---------------------------------------------------------------------------------		
cardenginei_arm7_twlsdk:
	@$(MAKE) -C cardenginei/arm7_twlsdk

#---------------------------------------------------------------------------------		
cardenginei_arm7_twlsdk3:
	@$(MAKE) -C cardenginei/arm7_twlsdk3

#---------------------------------------------------------------------------------		
cardenginei_arm7_dsiware:
	@$(MAKE) -C cardenginei/arm7_dsiware

#---------------------------------------------------------------------------------		
cardenginei_arm7_dsiware3:
	@$(MAKE) -C cardenginei/arm7_dsiware3

#---------------------------------------------------------------------------------		
cardenginei_arm7_cheat:
	@$(MAKE) -C cardenginei/arm7_cheat

#---------------------------------------------------------------------------------		
cardenginei_arm9_colorlut:
	@$(MAKE) -C cardenginei/arm9_colorlut

#---------------------------------------------------------------------------------		
cardenginei_arm9_igm:
	@$(MAKE) -C cardenginei/arm9_igm

#---------------------------------------------------------------------------------		
cardenginei_arm9:
	@$(MAKE) -C cardenginei/arm9

#---------------------------------------------------------------------------------		
cardenginei_arm9_gsdd:
	@$(MAKE) -C cardenginei/arm9_gsdd

#---------------------------------------------------------------------------------		
cardenginei_arm9_dldi:
	@$(MAKE) -C cardenginei/arm9_dldi

#---------------------------------------------------------------------------------		
cardenginei_arm9_gsdd_dldi:
	@$(MAKE) -C cardenginei/arm9_gsdd_dldi

#---------------------------------------------------------------------------------		
cardenginei_arm9_twlsdk:
	@$(MAKE) -C cardenginei/arm9_twlsdk

#---------------------------------------------------------------------------------		
cardenginei_arm9_twlsdk3:
	@$(MAKE) -C cardenginei/arm9_twlsdk3

#---------------------------------------------------------------------------------		
cardenginei_arm9_twlsdk_dldi:
	@$(MAKE) -C cardenginei/arm9_twlsdk_dldi

#---------------------------------------------------------------------------------		
cardenginei_arm9_twlsdk3_dldi:
	@$(MAKE) -C cardenginei/arm9_twlsdk3_dldi

#---------------------------------------------------------------------------------		
cardenginei_arm9_dsiware:
	@$(MAKE) -C cardenginei/arm9_dsiware

#---------------------------------------------------------------------------------		
cardenginei_arm9_dsiware3:
	@$(MAKE) -C cardenginei/arm9_dsiware3

#---------------------------------------------------------------------------------		
preLoadSettings:
	@$(MAKE) -C preLoadSettings

#---------------------------------------------------------------------------------		
apfix:
	@$(MAKE) -C apfix

#---------------------------------------------------------------------------------		
mobiclipOffsets:
	@$(MAKE) -C mobiclipOffsets

#---------------------------------------------------------------------------------		
dsi2dsSavePatches:
	@$(MAKE) -C dsi2dsSavePatches

#---------------------------------------------------------------------------------
#$(BUILD):
#	@[ -d $@ ] || mkdir -p $@
#	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).nds.orig.nds $(TARGET).arm9 $(BIN)
	@$(MAKE) -C arm7 clean
	@$(MAKE) -C arm9 clean
	@$(MAKE) -C cardengine/arm7 clean
	@$(MAKE) -C cardengine/arm7_music clean
	@$(MAKE) -C cardengine/arm9_igm clean
	@$(MAKE) -C cardengine/arm9_igm_extmem clean
	@$(MAKE) -C cardengine/arm9 clean
	@$(MAKE) -C cardengine/arm9_start clean
	@$(MAKE) -C cardengine/arm9_start_foto clean
	@$(MAKE) -C cardengine/arm9_alt clean
	@$(MAKE) -C cardengine/arm9_alt_gsdd clean
	@$(MAKE) -C cardengine/arm9_alt2 clean
	@$(MAKE) -C cardengine/arm9_alt3 clean
	@$(MAKE) -C cardengine/arm9_extmem clean
	@$(MAKE) -C cardengine/arm9_extmem_gsdd clean
	@$(MAKE) -C cardengine/arm9_extmem_start clean
	@$(MAKE) -C cardengine/arm9_extmem_foto clean
	@$(MAKE) -C cardenginei/arm7 clean
	@$(MAKE) -C cardenginei/arm7_alt clean
	@$(MAKE) -C cardenginei/arm7_twlsdk clean
	@$(MAKE) -C cardenginei/arm7_twlsdk3 clean
	@$(MAKE) -C cardenginei/arm7_dsiware clean
	@$(MAKE) -C cardenginei/arm7_dsiware3 clean
	@$(MAKE) -C cardenginei/arm7_cheat clean
	@$(MAKE) -C cardenginei/arm7_cheatonly clean
	@$(MAKE) -C cardenginei/arm9_colorlut clean
	@$(MAKE) -C cardenginei/arm9_igm clean
	@$(MAKE) -C cardenginei/arm9 clean
	@$(MAKE) -C cardenginei/arm9_gsdd clean
	@$(MAKE) -C cardenginei/arm9_dldi clean
	@$(MAKE) -C cardenginei/arm9_gsdd_dldi clean
	@$(MAKE) -C cardenginei/arm9_twlsdk clean
	@$(MAKE) -C cardenginei/arm9_twlsdk_dldi clean
	@$(MAKE) -C cardenginei/arm9_twlsdk3 clean
	@$(MAKE) -C cardenginei/arm9_twlsdk3_dldi clean
	@$(MAKE) -C cardenginei/arm9_dsiware clean
	@$(MAKE) -C cardenginei/arm9_dsiware3 clean
	@rm -f $(NITRODATA)/preLoadSettingsMEP.pck
	@rm -f $(NITRODATA)/preLoadSettingsDSi.pck
	@rm -f $(NITRODATA)/preLoadSettings3DS.pck
	@rm -f $(NITRODATA)/apfix.pck
	@rm -f $(NITRODATA)/mobiclipOffsets.pck
	@rm -f $(NITRODATA)/dsi2dsSavePatches.pck
	@$(MAKE) -C bootloader clean
	@$(MAKE) -C bootloader2 clean
	@$(MAKE) -C bootloaderi clean
	@$(MAKE) -C bootloaderi2 clean

$(BIN):
	@mkdir -p $(BIN)

#---------------------------------------------------------------------------------
else

#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------