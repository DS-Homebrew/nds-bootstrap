ifneq (,$(shell which python3))
PYTHON	:= python3
else ifneq (,$(shell which python2))
PYTHON	:= python2
else ifneq (,$(shell which python))
PYTHON	:= python
else
$(error "Python not found in PATH, please install it.")
endif

#---------------------------------------------------------------------------------
# OUTPUT_DIR is the directory where final published files will be placed
#---------------------------------------------------------------------------------
OUTPUT_DIR		:=	../nitrofiles

FILESDSI		:= files-dsi
FILES3DS		:= files-3ds

#---------------------------------------------------------------------------------
# Goals for Build
#---------------------------------------------------------------------------------
.PHONY:

all:	$(OUTPUT_DIR)/preLoadSettingsDSi.pck $(OUTPUT_DIR)/preLoadSettings3DS.pck

$(OUTPUT_DIR)/preLoadSettingsDSi.pck: $(FILESDSI)
	$(PYTHON) pack.py $^ -o $@

$(OUTPUT_DIR)/preLoadSettings3DS.pck: $(FILES3DS)
	$(PYTHON) pack.py $^ -o $@
