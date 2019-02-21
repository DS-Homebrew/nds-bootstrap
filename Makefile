#---------------------------------------------------------------------------------
# PACKAGE is the directory where final published files will be placed
#---------------------------------------------------------------------------------
PACKAGE		:=	bin

#---------------------------------------------------------------------------------
# Goals for Build
#---------------------------------------------------------------------------------
.PHONY: all package release nightly retail

all:	retail

package-release: release
	@mkdir -p "$(PACKAGE)"
	@cp "retail/bin/nds-bootstrap-release.nds" "$(PACKAGE)/nds-bootstrap-release.nds"

package-nightly: nightly
	@mkdir -p "$(PACKAGE)"
	@cp "retail/bin/nds-bootstrap-nightly.nds" "$(PACKAGE)/nds-bootstrap-nightly.nds"

release:
	@$(MAKE) -C retail release

nightly:
	@$(MAKE) -C retail nightly

retail:
	@$(MAKE) -C retail

clean:
	@echo clean build direcotiries
	@$(MAKE) -C retail clean

	@echo clean package files
	@rm -rf "bin"
