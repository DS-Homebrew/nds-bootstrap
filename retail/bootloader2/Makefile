#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

-include $(DEVKITARM)/ds_rules

ifeq ($(OS),Windows_NT)
LZSS			?=	../../../lzss.exe

else

LZSS			?=	lzss

endif
#---------------------------------------------------------------------------------
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	load2
BUILD		:=	build
SOURCES		:=	../bootloader/source ../bootloader/source/arm7 ../bootloader/source/arm7/libnds ../bootloader/source/arm7/save_patches ../bootloader/source/arm9 ../common/source ../common/source_general
INCLUDES	:=	../bootloader/include build ../common/include ../common/include_fat
DATA		:=	../data
SPECS		:=  specs

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=  -mthumb -march=armv4t

CFLAGS	:=	-g -Wall -O2\
			-mcpu=arm7tdmi -mtune=arm7tdmi -fomit-frame-pointer\
			-ffast-math \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM7 -D_NO_SDMMC -DB4DS -DLOADERTYPE2 -std=gnu99

ASFLAGS	:=	-g $(ARCH) $(INCLUDE)
LDFLAGS	=	-nostartfiles -T $(TOPDIR)/load.ld -g $(ARCH) -Wl,--nmagic -Wl,-Map,$(TARGET).map

LIBS	:=

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
 
export TOPDIR	:=	$(CURDIR)
export LOADBIN	:=	$(CURDIR)/../nitrofiles_dec/$(TARGET).bin
export LOADBIN_CMP	:=	$(CURDIR)/../nitrofiles/$(TARGET).lz77
export LOADELF	:=	$(CURDIR)/$(TARGET).elf
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
 
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
#BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))
BINFILES	:=	
 
export OFILES	:=	$(addsuffix .o,$(BINFILES)) $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
 
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)
 
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

#---------------------------------------------------------------------------------
# use CC for linking standard C
#---------------------------------------------------------------------------------
export LD	:=	$(CC)
#---------------------------------------------------------------------------------

.PHONY: $(BUILD) clean
 
#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) *.elf
 
 
#---------------------------------------------------------------------------------
else
 
DEPENDS	:=	$(OFILES:.o=.d)
 
#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(LOADBIN_CMP): $(LOADBIN)
	@cp $< $@
	@$(LZSS) -ewn $@

$(LOADBIN)	:	$(LOADELF)


$(LOADELF)	:	$(OFILES)
	@echo linking $(notdir $@)
	@$(LD)  $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	
	
-include $(DEPENDS)


#---------------------------------------------------------------------------------
%.bin: %.elf
#---------------------------------------------------------------------------------
	@$(OBJCOPY) -O binary $< $@
	@echo built ... $(notdir $@)

#---------------------------------------------------------------------------------
%.bin.o: %.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
