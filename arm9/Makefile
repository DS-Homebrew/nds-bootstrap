#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
# MAXMOD_SOUNDBANK contains a directory of music and sound effect files
#---------------------------------------------------------------------------------
TARGET		:=	nds-bootstrap
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	include
DATA		:=	../data
#GRAPHICS	:=  graphics
IMAGES		:=	graphics
MUSIC       :=  music

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-mthumb -mthumb-interwork

CFLAGS	:=	-g -Wall -O2\
 		-march=armv5te -mtune=arm946e-s -fomit-frame-pointer\
		-ffast-math \
		$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM9 -std=gnu99
CXXFLAGS	:= $(CFLAGS) -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project (order is important)
#---------------------------------------------------------------------------------
LIBS	:=	-lfat -lnds9
 
 
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

export OUTPUT	:=	$(CURDIR)/$(TARGET)

#export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
#					$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
#					$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir))

export AUDIOFILES	:=	$(foreach dir,$(notdir $(wildcard $(MUSIC)/*.*)),$(CURDIR)/$(MUSIC)/$(dir))

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
#PNGFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.png)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*))) soundbank.bin
 
BMPFILES	:=	$(foreach dir,$(IMAGES),$(notdir $(wildcard $(dir)/*.bmp)))
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
					$(BMPFILES:.bmp=.o) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
 
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)
 
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)


export OUTPUT	:=	$(CURDIR)/$(TARGET)
 
.PHONY: $(BUILD) clean

all : $(BUILD)
 
#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
 
#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds *.bin 

#---------------------------------------------------------------------------------
else
 
DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).elf	:	$(OFILES)
 
#---------------------------------------------------------------------------------
# graphics
#---------------------------------------------------------------------------------
#Cartridge Wait Prompt (bottom screen)
CartPrompt01.s			:	../$(IMAGES)/CartPrompt01.bmp
	grit	$< -gB8 -gzl -fts -gTff00ff -o $@ -q

CartPrompt02.s			:	../$(IMAGES)/CartPrompt02.bmp
	grit	$< -gB8 -gzl -fts -gTff00ff -o $@ -q

CartPrompt03.s			:	../$(IMAGES)/CartPrompt03.bmp
	grit	$< -gB8 -gzl -fts -gTff00ff -o $@ -q

CartPrompt04.s			:	../$(IMAGES)/CartPrompt04.bmp
	grit	$< -gB8 -gzl -fts -gTff00ff -o $@ -q

# generic fade from white to black
fade00.s			:	../$(IMAGES)/fade00.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

fade01.s			:	../$(IMAGES)/fade01.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

fade02.s			:	../$(IMAGES)/fade02.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

fade03.s			:	../$(IMAGES)/fade03.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

fade04.s			:	../$(IMAGES)/fade04.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

# generic bottom screen error message
suberror00.s			:	../$(IMAGES)/suberror00.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q
	
suberror01.s			:	../$(IMAGES)/suberror01.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

suberror02.s			:	../$(IMAGES)/suberror02.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

suberror03.s			:	../$(IMAGES)/suberror03.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

suberror04.s			:	../$(IMAGES)/suberror04.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

suberror05.s			:	../$(IMAGES)/suberror05.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

suberror06.s			:	../$(IMAGES)/suberror06.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

# No bit31 patch error
toperror1_00.s			:	../$(IMAGES)/toperror1_00.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror1_01.s			:	../$(IMAGES)/toperror1_01.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror1_02.s			:	../$(IMAGES)/toperror1_02.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror1_03.s			:	../$(IMAGES)/toperror1_03.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror1_04.s			:	../$(IMAGES)/toperror1_04.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror1_05.s			:	../$(IMAGES)/toperror1_05.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror1_06.s			:	../$(IMAGES)/toperror1_06.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

# No Cartridge Error
toperror2_00.s			:	../$(IMAGES)/toperror2_00.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror2_01.s			:	../$(IMAGES)/toperror2_01.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror2_02.s			:	../$(IMAGES)/toperror2_02.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror2_03.s			:	../$(IMAGES)/toperror2_03.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror2_04.s			:	../$(IMAGES)/toperror2_04.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror2_05.s			:	../$(IMAGES)/toperror2_05.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

toperror2_06.s			:	../$(IMAGES)/toperror2_06.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

# Main Boot Splash sequence
Bot00.s			:	../$(IMAGES)/Bot00.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Bot01.s			:	../$(IMAGES)/Bot01.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Bot02.s			:	../$(IMAGES)/Bot02.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Bot03.s			:	../$(IMAGES)/Bot03.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Bot04.s			:	../$(IMAGES)/Bot04.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Bot05.s			:	../$(IMAGES)/Bot05.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Bot06.s			:	../$(IMAGES)/Bot06.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Bot07.s			:	../$(IMAGES)/Bot07.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Bot08.s			:	../$(IMAGES)/Bot08.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Bot09.s			:	../$(IMAGES)/Bot09.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Bot10.s			:	../$(IMAGES)/Bot10.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top00.s			:	../$(IMAGES)/Top00.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top01.s			:	../$(IMAGES)/Top01.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top02.s			:	../$(IMAGES)/Top02.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top03.s			:	../$(IMAGES)/Top03.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top04.s			:	../$(IMAGES)/Top04.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top05.s			:	../$(IMAGES)/Top05.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top06.s			:	../$(IMAGES)/Top06.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top07.s			:	../$(IMAGES)/Top07.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top08.s			:	../$(IMAGES)/Top08.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top09.s			:	../$(IMAGES)/Top09.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top10.s			:	../$(IMAGES)/Top10.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top11.s			:	../$(IMAGES)/Top11.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top12.s			:	../$(IMAGES)/Top12.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top13.s			:	../$(IMAGES)/Top13.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top14.s			:	../$(IMAGES)/Top14.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top15.s			:	../$(IMAGES)/Top15.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top16.s			:	../$(IMAGES)/Top16.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top17.s			:	../$(IMAGES)/Top17.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top18.s			:	../$(IMAGES)/Top18.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top19.s			:	../$(IMAGES)/Top19.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top20.s			:	../$(IMAGES)/Top20.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top21.s			:	../$(IMAGES)/Top21.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top22.s			:	../$(IMAGES)/Top22.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top23.s			:	../$(IMAGES)/Top23.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top24.s			:	../$(IMAGES)/Top24.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top25.s			:	../$(IMAGES)/Top25.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top26.s			:	../$(IMAGES)/Top26.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top27.s			:	../$(IMAGES)/Top27.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top28.s			:	../$(IMAGES)/Top28.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top29.s			:	../$(IMAGES)/Top29.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top30.s			:	../$(IMAGES)/Top30.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top31.s			:	../$(IMAGES)/Top31.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top32.s			:	../$(IMAGES)/Top32.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top33.s			:	../$(IMAGES)/Top33.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top34.s			:	../$(IMAGES)/Top34.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top35.s			:	../$(IMAGES)/Top35.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top36.s			:	../$(IMAGES)/Top36.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

Top37.s			:	../$(IMAGES)/Top37.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

# DSi BootSplash. Used if TWL mode detected
DSi18.s			:	../$(IMAGES)/DSi18.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi19.s			:	../$(IMAGES)/DSi19.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi20.s			:	../$(IMAGES)/DSi20.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi21.s			:	../$(IMAGES)/DSi21.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi22.s			:	../$(IMAGES)/DSi22.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi23.s			:	../$(IMAGES)/DSi23.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi24.s			:	../$(IMAGES)/DSi24.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi25.s			:	../$(IMAGES)/DSi25.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi26.s			:	../$(IMAGES)/DSi26.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi27.s			:	../$(IMAGES)/DSi27.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi28.s			:	../$(IMAGES)/DSi28.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi29.s			:	../$(IMAGES)/DSi29.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi30.s			:	../$(IMAGES)/DSi30.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi31.s			:	../$(IMAGES)/DSi31.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi32.s			:	../$(IMAGES)/DSi32.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi33.s			:	../$(IMAGES)/DSi33.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi34.s			:	../$(IMAGES)/DSi34.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi35.s			:	../$(IMAGES)/DSi35.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

DSi36.s			:	../$(IMAGES)/DSi36.bmp
	grit	$< -gB8 -gzl -gTff00ff -fts -o $@ -q

#---------------------------------------------------------------------------------
# rule to build soundbank from music files
#---------------------------------------------------------------------------------
soundbank.bin : $(AUDIOFILES)
#---------------------------------------------------------------------------------
	@mmutil $^ -d -osoundbank.bin -hsoundbank.h

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
#%.s %.h   : %.png %.grit
#---------------------------------------------------------------------------------
#	grit $< -fts -o$*
 
#-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
