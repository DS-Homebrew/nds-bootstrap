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

FILESMEP		:= files-mep
FILESDSI		:= files-dsi
FILES3DS		:= files-3ds
FILES3DSEN		:= files-3ds-en
FILES3DSFR		:= files-3ds-fr
FILES3DSDE		:= files-3ds-de
FILES3DSIT		:= files-3ds-it
FILES3DSES		:= files-3ds-es

#---------------------------------------------------------------------------------
# Goals for Build
#---------------------------------------------------------------------------------
.PHONY:

all:	$(OUTPUT_DIR)/preLoadSettingsMEP.pck $(OUTPUT_DIR)/preLoadSettingsDSi.pck $(OUTPUT_DIR)/preLoadSettings3DS.pck $(OUTPUT_DIR)/preLoadSettings3DS-en.pck $(OUTPUT_DIR)/preLoadSettings3DS-fr.pck $(OUTPUT_DIR)/preLoadSettings3DS-de.pck $(OUTPUT_DIR)/preLoadSettings3DS-it.pck $(OUTPUT_DIR)/preLoadSettings3DS-es.pck

$(OUTPUT_DIR)/preLoadSettingsMEP.pck: $(FILESMEP)
	$(PYTHON) pack.py $^ -o $@

$(OUTPUT_DIR)/preLoadSettingsDSi.pck: $(FILESDSI)
	$(PYTHON) pack.py $^ -o $@

$(OUTPUT_DIR)/preLoadSettings3DS.pck: $(FILES3DS)
	$(PYTHON) pack.py $^ -o $@

$(OUTPUT_DIR)/preLoadSettings3DS-en.pck: $(FILES3DSEN)
	$(PYTHON) pack.py $^ -o $@

$(OUTPUT_DIR)/preLoadSettings3DS-fr.pck: $(FILES3DSFR)
	$(PYTHON) pack.py $^ -o $@

$(OUTPUT_DIR)/preLoadSettings3DS-de.pck: $(FILES3DSDE)
	$(PYTHON) pack.py $^ -o $@

$(OUTPUT_DIR)/preLoadSettings3DS-it.pck: $(FILES3DSIT)
	$(PYTHON) pack.py $^ -o $@

$(OUTPUT_DIR)/preLoadSettings3DS-es.pck: $(FILES3DSES)
	$(PYTHON) pack.py $^ -o $@
