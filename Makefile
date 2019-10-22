#---------------------------------------------------------------------------------
# PACKAGE is the directory where final published files will be placed
#---------------------------------------------------------------------------------
PACKAGE		:=	bin

#---------------------------------------------------------------------------------
# Goals for Build
#---------------------------------------------------------------------------------
.PHONY: all package release nightly b4ds retail hb

all:	b4ds retail hb

package-release: release
	@mkdir -p "$(PACKAGE)"
	@cp "b4ds/bin/b4ds-release.nds" "$(PACKAGE)/b4ds-release.nds"
	@cp "retail/bin/nds-bootstrap-release.nds" "$(PACKAGE)/nds-bootstrap-release.nds"
	@cp "hb/bin/nds-bootstrap-hb-release.nds" "$(PACKAGE)/nds-bootstrap-hb-release.nds"

package-nightly: nightly
	@mkdir -p "$(PACKAGE)"
	@cp "b4ds/bin/b4ds-nightly.nds" "$(PACKAGE)/b4ds-nightly.nds"
	@cp "retail/bin/nds-bootstrap-nightly.nds" "$(PACKAGE)/nds-bootstrap-nightly.nds"
	@cp "hb/bin/nds-bootstrap-hb-nightly.nds" "$(PACKAGE)/nds-bootstrap-hb-nightly.nds"

release:
	@$(MAKE) -C b4ds release
	@$(MAKE) -C retail release
	@$(MAKE) -C hb release

nightly:
	@$(MAKE) -C b4ds nightly
	@$(MAKE) -C retail nightly
	@$(MAKE) -C hb nightly

b4ds:
	@$(MAKE) -C b4ds

retail:
	@$(MAKE) -C retail

hb:
	@$(MAKE) -C hb

clean:
	@echo clean build direcotiries
	@$(MAKE) -C b4ds clean
	@$(MAKE) -C retail clean
	@$(MAKE) -C hb clean

	@echo clean package files
	@rm -rf "bin"
