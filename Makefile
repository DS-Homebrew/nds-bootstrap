#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
.SECONDARY:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

# These set the information text in the nds file
GAME_TITLE     := NDS BOOTSTRAP
GAME_SUBTITLE1 := Runs an .nds file
GAME_SUBTITLE2 := Made by Ahezard

include $(DEVKITARM)/ds_rules

export HBMENU_MAJOR	:= 0
export HBMENU_MINOR	:= 5
export HBMENU_PATCH	:= 0


VERSION	:=	$(HBMENU_MAJOR).$(HBMENU_MINOR).$(HBMENU_PATCH)
#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
# DATA is a list of directories containing binary files embedded using bin2o
# GRAPHICS is a list of directories containing image files to be converted with grit
#---------------------------------------------------------------------------------
TARGET		:=	nds-bootstrap
BIN			:=	bin
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	include
DATA		:=	data
ASSETS		:=	assets
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

export OUTPUT	:=	$(CURDIR)/$(BIN)/$(TARGET)

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
BINFILES	:=	load.bin bootstub.bin

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
.PHONY: all dist nightly bootloader cardengine_arm7 cardengine_arm9 clean

all:	$(OUTPUT).nds

dist:	all
#	@rm	-fr	hbmenu
#	@mkdir hbmenu
#	@cp hbmenu.nds hbmenu/BOOT.NDS
#	@cp BootStrap/_BOOT_MP.NDS BootStrap/TTMENU.DAT BootStrap/_DS_MENU.DAT BootStrap/ez5sys.bin BootStrap/akmenu4.nds hbmenu
#	@tar -cvjf hbmenu-$(VERSION).tar.bz2 hbmenu testfiles README.md COPYING -X exclude.lst

nightly:	$(OUTPUT).nds
	@rm -f $(CURDIR)/$(BIN)/nightly-bootstrap.nds
	@rm -f $(CURDIR)/$(BIN)/nightly-bootstrap-sdk5.nds
	@cp $(OUTPUT).nds $(CURDIR)/$(BIN)/nightly-bootstrap.nds
	@cp $(OUTPUT).nds $(CURDIR)/$(BIN)/nightly-bootstrap-sdk5.nds

$(OUTPUT).nds:	$(BIN) arm7/$(TARGET).elf arm9/$(TARGET).elf
	ndstool	-c $(OUTPUT).nds -7 arm7/$(TARGET).elf -9 arm9/$(TARGET).elf \
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
	@$(MAKE) -C cardengine/arm9 clean
	@$(MAKE) -C bootloader clean
		
$(DATA):
	@mkdir -p $(DATA)

$(BIN):
	@mkdir -p $(BIN)

#---------------------------------------------------------------------------------
else

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
#$(OUTPUT).nds	: 	$(OUTPUT).elf
#$(OUTPUT).elf	:	$(OFILES)

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
