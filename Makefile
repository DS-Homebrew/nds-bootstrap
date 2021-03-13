#---------------------------------------------------------------------------------
# PACKAGE is the directory where final published files will be placed
#---------------------------------------------------------------------------------
PACKAGE		:=	bin

#---------------------------------------------------------------------------------
# Goals for Build
#---------------------------------------------------------------------------------
.PHONY: all package release nightly retail hb

all:	retail hb

package-release: release
	@mkdir -p "$(PACKAGE)"
	@cp "retail/bin/nds-bootstrap-release.nds" "$(PACKAGE)/nds-bootstrap-release.nds"
	@cp "hb/bin/nds-bootstrap-hb-release.nds" "$(PACKAGE)/nds-bootstrap-hb-release.nds"

package-nightly: nightly
	@mkdir -p "$(PACKAGE)"
	@cp "retail/bin/nds-bootstrap-nightly.nds" "$(PACKAGE)/nds-bootstrap-nightly.nds"
	@cp "hb/bin/nds-bootstrap-hb-nightly.nds" "$(PACKAGE)/nds-bootstrap-hb-nightly.nds"

release:
	@$(MAKE) -C retail release
	@$(MAKE) -C hb release

nightly:
	@$(MAKE) -C retail nightly
	@$(MAKE) -C hb nightly

retail:
	@$(MAKE) -C retail

hb:
	@$(MAKE) -C hb

clean:
	@echo clean build direcotiries
	@$(MAKE) -C retail clean
	@$(MAKE) -C hb clean

	@echo clean package files
	@rm -rf "bin"
