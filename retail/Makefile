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
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	include
DATA		:=	data
ASSETS		:=	assets
NITRODATA	:=	nitrofiles
GRAPHICS	:=  $(ASSETS)/gfx

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-mthumb -mthumb-interwork

COMMON	:=	-g -Wall -O2\
 		-march=armv5te -mtune=arm946e-s -fomit-frame-pointer\
		-ffast-math \
		$(ARCH)

COMMON	+=	$(INCLUDE) -DARM9
CFLAGS	:=	$(COMMON) -std=gnu99
CXXFLAGS	:= $(COMMON) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm9.specs -g -Wl,--gc-sections $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project (order is important)
#---------------------------------------------------------------------------------
LIBS	:= 	-lfat -lnds9


#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBNDS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(BIN)/$(TARGET).nds

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
					$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

export CC		:=	$(PREFIX)gcc
export CXX		:=	$(PREFIX)g++
export AR		:=	$(PREFIX)ar
export OBJCOPY	:=	$(PREFIX)objcopy
export CPP		:=	$(PREFIX)cpp

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
PNGFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.png)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	

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

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(PNGFILES:.png=.o) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

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
.PHONY: all dist release nightly bootloader bootloader2 bootloaderi bootloaderi2 cardengine_arm7 cardengine_arm9_igm cardengine_arm9_igm_extmem cardengine_arm9 cardengine_arm9_start cardengine_arm9_alt cardengine_arm9_alt2 cardengine_arm9_extmem cardenginei_arm7 cardenginei_arm7_alt cardenginei_arm7_twlsdk cardenginei_arm7_dsiware cardenginei_arm7_cheat cardenginei_arm9_igm cardenginei_arm9 cardenginei_arm9_alt cardenginei_arm9_dldi cardenginei_arm9_twlsdk cardenginei_arm9_twlsdk3 cardenginei_arm9_twlsdk_dldi cardenginei_arm9_twlsdk3_dldi cardenginei_arm9_dsiware cardenginei_arm9_romInRam arm7/$(TARGET).elf arm9/$(TARGET).elf clean

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
arm9/$(TARGET).elf:	bootloader bootloader2 bootloaderi bootloaderi2
	@$(MAKE) -C arm9

#---------------------------------------------------------------------------------		
bootloader: $(DATA) cardengine_arm7 cardengine_arm9_igm cardengine_arm9_igm_extmem cardengine_arm9 cardengine_arm9_start cardengine_arm9_alt cardengine_arm9_alt2 cardengine_arm9_extmem
	@$(MAKE) -C bootloader

#---------------------------------------------------------------------------------		
bootloader2: $(DATA)
	@$(MAKE) -C bootloader2

#---------------------------------------------------------------------------------		
bootloaderi: $(DATA) cardenginei_arm7 cardenginei_arm7_alt cardenginei_arm7_twlsdk cardenginei_arm7_dsiware cardenginei_arm7_cheat cardenginei_arm9_igm cardenginei_arm9 cardenginei_arm9_alt cardenginei_arm9_dldi cardenginei_arm9_twlsdk cardenginei_arm9_twlsdk3 cardenginei_arm9_twlsdk_dldi cardenginei_arm9_twlsdk3_dldi cardenginei_arm9_dsiware cardenginei_arm9_romInRam
	@$(MAKE) -C bootloaderi

#---------------------------------------------------------------------------------		
bootloaderi2: $(DATA)
	@$(MAKE) -C bootloaderi2

#---------------------------------------------------------------------------------		
cardengine_arm7: $(DATA)
	@$(MAKE) -C cardengine/arm7

#---------------------------------------------------------------------------------		
cardengine_arm9_igm: $(DATA)
	@$(MAKE) -C cardengine/arm9_igm

#---------------------------------------------------------------------------------		
cardengine_arm9_igm_extmem: $(DATA)
	@$(MAKE) -C cardengine/arm9_igm_extmem

#---------------------------------------------------------------------------------		
cardengine_arm9: $(DATA)
	@$(MAKE) -C cardengine/arm9

#---------------------------------------------------------------------------------		
cardengine_arm9_start: $(DATA)
	@$(MAKE) -C cardengine/arm9_start

#---------------------------------------------------------------------------------		
cardengine_arm9_alt: $(DATA)
	@$(MAKE) -C cardengine/arm9_alt

#---------------------------------------------------------------------------------		
cardengine_arm9_alt2: $(DATA)
	@$(MAKE) -C cardengine/arm9_alt2

#---------------------------------------------------------------------------------		
cardengine_arm9_extmem: $(DATA)
	@$(MAKE) -C cardengine/arm9_extmem

#---------------------------------------------------------------------------------		
cardenginei_arm7: $(DATA)
	@$(MAKE) -C cardenginei/arm7

#---------------------------------------------------------------------------------		
cardenginei_arm7_alt: $(DATA)
	@$(MAKE) -C cardenginei/arm7_alt

#---------------------------------------------------------------------------------		
cardenginei_arm7_twlsdk: $(DATA)
	@$(MAKE) -C cardenginei/arm7_twlsdk

#---------------------------------------------------------------------------------		
cardenginei_arm7_dsiware: $(DATA)
	@$(MAKE) -C cardenginei/arm7_dsiware

#---------------------------------------------------------------------------------		
cardenginei_arm7_cheat: $(DATA)
	@$(MAKE) -C cardenginei/arm7_cheat

#---------------------------------------------------------------------------------		
cardenginei_arm9_igm: $(DATA)
	@$(MAKE) -C cardenginei/arm9_igm

#---------------------------------------------------------------------------------		
cardenginei_arm9: $(DATA)
	@$(MAKE) -C cardenginei/arm9

#---------------------------------------------------------------------------------		
cardenginei_arm9_alt: $(DATA)
	@$(MAKE) -C cardenginei/arm9_alt

#---------------------------------------------------------------------------------		
cardenginei_arm9_dldi: $(DATA)
	@$(MAKE) -C cardenginei/arm9_dldi

#---------------------------------------------------------------------------------		
cardenginei_arm9_twlsdk: $(DATA)
	@$(MAKE) -C cardenginei/arm9_twlsdk

#---------------------------------------------------------------------------------		
cardenginei_arm9_twlsdk3: $(DATA)
	@$(MAKE) -C cardenginei/arm9_twlsdk3

#---------------------------------------------------------------------------------		
cardenginei_arm9_twlsdk_dldi: $(DATA)
	@$(MAKE) -C cardenginei/arm9_twlsdk_dldi

#---------------------------------------------------------------------------------		
cardenginei_arm9_twlsdk3_dldi: $(DATA)
	@$(MAKE) -C cardenginei/arm9_twlsdk3_dldi

#---------------------------------------------------------------------------------		
cardenginei_arm9_dsiware: $(DATA)
	@$(MAKE) -C cardenginei/arm9_dsiware

#---------------------------------------------------------------------------------		
cardenginei_arm9_romInRam: $(DATA)
	@$(MAKE) -C cardenginei/arm9_romInRam

#---------------------------------------------------------------------------------
#$(BUILD):
#	@[ -d $@ ] || mkdir -p $@
#	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).nds.orig.nds $(TARGET).arm9 $(DATA) $(BIN)
	@$(MAKE) -C arm7 clean
	@$(MAKE) -C arm9 clean
	@$(MAKE) -C cardengine/arm7 clean
	@$(MAKE) -C cardengine/arm9_igm clean
	@$(MAKE) -C cardengine/arm9_igm_extmem clean
	@$(MAKE) -C cardengine/arm9 clean
	@$(MAKE) -C cardengine/arm9_start clean
	@$(MAKE) -C cardengine/arm9_alt clean
	@$(MAKE) -C cardengine/arm9_alt2 clean
	@$(MAKE) -C cardengine/arm9_extmem clean
	@$(MAKE) -C cardenginei/arm7 clean
	@$(MAKE) -C cardenginei/arm7_alt clean
	@$(MAKE) -C cardenginei/arm7_twlsdk clean
	@$(MAKE) -C cardenginei/arm7_dsiware clean
	@$(MAKE) -C cardenginei/arm7_cheat clean
	@$(MAKE) -C cardenginei/arm7_cheatonly clean
	@$(MAKE) -C cardenginei/arm9_igm clean
	@$(MAKE) -C cardenginei/arm9 clean
	@$(MAKE) -C cardenginei/arm9_alt clean
	@$(MAKE) -C cardenginei/arm9_dldi clean
	@$(MAKE) -C cardenginei/arm9_twlsdk clean
	@$(MAKE) -C cardenginei/arm9_twlsdk_dldi clean
	@$(MAKE) -C cardenginei/arm9_twlsdk3 clean
	@$(MAKE) -C cardenginei/arm9_twlsdk3_dldi clean
	@$(MAKE) -C cardenginei/arm9_dsiware clean
	@$(MAKE) -C cardenginei/arm9_romInRam clean
	@$(MAKE) -C bootloader clean
	@$(MAKE) -C bootloader2 clean
	@$(MAKE) -C bootloaderi clean
	@$(MAKE) -C bootloaderi2 clean

$(DATA):
	@mkdir -p $(DATA)

$(BIN):
	@mkdir -p $(BIN)

#---------------------------------------------------------------------------------
else

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
#$(OUTPUT)		: 	$(TARGET).elf
#$(TARGET).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)

#---------------------------------------------------------------------------------
# This rule creates assembly source files using grit
# grit takes an image file and a .grit describing how the file is to be processed
# add additional rules like this for each image extension
# you use in the graphics folders
#---------------------------------------------------------------------------------
%.s %.h   : %.png %.grit
#---------------------------------------------------------------------------------
	grit $< -fts -o$*

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------