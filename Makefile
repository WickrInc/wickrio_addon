WICKR_SDK = wickr-sdk
LOCALREPO = localRepo/$(WICKR_SDK)
SDK_BRANCH = master

ifeq ($(OS),Windows_NT)
    DIR := $(subst C:,,${CURDIR})
    HEADERDIR = "$(subst /,\\,$(DIR)/export/Wickr)"
else
    DIR := ${CURDIR}
    HEADERDIR = $(DIR)/export/Wickr
endif
HEADER_START = "*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*"

default: all
	@echo $(DIR)

update: sdk.update

clean: osx.clean
	rm -rf $(HEADERDIR)

$(WICKR_SDK)/wickr-sdk.pro:
	@echo $(HEADER_START)
	@echo "Starting to install wickr-sdk from GIT"
	git status
	git submodule init wickr-sdk
	git submodule update --recursive wickr-sdk
	cd $(WICKR_SDK); git checkout $(SDK_BRANCH); git pull
	cd $(WICKR_SDK); make
	@echo "Finished to install wickr-sdk from GIT"
	@echo $(HEADER_END)

install.headers:
	@echo "Start copying headers to" $(HEADERDIR)
	mkdir -p $(HEADERDIR)
	@cd $(WICKR_SDK)/src; cp *.h $(HEADERDIR);
	@cd $(WICKR_SDK)/wccorebson/exports/; cp *.h $(HEADERDIR);
	mkdir -p $(HEADERDIR)/aes
	@cd $(WICKR_SDK)/src/aes; cp *.h $(HEADERDIR)/aes
	mkdir -p $(HEADERDIR)/crypto
	@cd $(WICKR_SDK)/src/crypto; cp *.h $(HEADERDIR)/crypto
	mkdir -p $(HEADERDIR)/strings
	@cd $(WICKR_SDK)/src/strings; cp *.h $(HEADERDIR)/strings
	mkdir -p $(HEADERDIR)/hashing
	@cd $(WICKR_SDK)/src/hashing; cp *.h $(HEADERDIR)/hashing
	mkdir -p $(HEADERDIR)/cryptobinding
	@cd $(WICKR_SDK)/src/cryptobinding; cp *.h $(HEADERDIR)/cryptobinding
	@echo "Done copying headers to" $(HEADERDIR)

sdk.update:
	@echo $(HEADER_START)
	@echo "Update the submodules"
	@echo "Starting to update wickr-sdk"
	cd $(WICKR_SDK); git checkout $(SDK_BRANCH); git pull; make; make update
	@echo $(HEADER_END)

##########################################################
# OSX build

osx:
	cd $(WICKR_SDK); make osx

osx.release:
	cd $(WICKR_SDK); make osx.release

osx.clean:
	cd $(WICKR_SDK); make osx.clean

osx.install: osx install.headers
	@cd $(WICKR_SDK); make osx.install

osx.release.install: osx.release install.headers
	@cd $(WICKR_SDK); make osx.release.install


##########################################################
# Windows 32 build

win32:
	cd $(WICKR_SDK); make win32

win32.release:
	cd $(WICKR_SDK); make win32.release

win32.install: win32 install.headers
	@cd $(WICKR_SDK); make win32.install

win32.release.install: win32.release install.headers
	@cd $(WICKR_SDK); make win32.release.install

win32.clean:
	cd $(WICKR_SDK); make win32.clean
	# TODO: Remove the libraries

##########################################################
# Linux build

linux:
	cd $(WICKR_SDK); make linux

linux.release:
	cd $(WICKR_SDK); make linux.release

linux.clean:
	cd $(WICKR_SDK); make linux.clean

linux.install: linux install.headers
	@cd $(WICKR_SDK); make linux.install

linux.release.install: linux.release install.headers
	@cd $(WICKR_SDK); make linux.release.install


##########################################################
all: $(WICKR_SDK)/wickr-sdk.pro
	@echo done!
