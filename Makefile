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
WICKR_SDK_OSX = $(WICKR_SDK)/build-osx
WICKR_SDK_OSX_DEB = $(WICKR_SDK)/build-osx/.debug
WICKR_SDK_OSX_REL = $(WICKR_SDK)/build-osx/.release

osx:
	cd $(WICKR_SDK); make osx

osx.release:
	cd $(WICKR_SDK); make osx.release

osx.clean:
	cd $(WICKR_SDK); make osx.clean

# OSX installation definitions
WICKR_DTOP_PLATFORM_OSX_DEB = $(DIR)/platforms/mac/lib64/debug
WICKR_DTOP_PLATFORM_OSX_REL = $(DIR)/platforms/mac/lib64/release

osx.install: osx install.headers
	@echo "Moving OSX (debug) libraries to" $(WICKR_DTOP_PLATFORM_OSX_DEB)
	mkdir -p $(WICKR_DTOP_PLATFORM_OSX_DEB)
	@cd $(WICKR_SDK); cp products/debug/libwickrcore.a $(WICKR_DTOP_PLATFORM_OSX_DEB)

osx.release.install: osx.release install.headers
	@echo "Moving OSX (release) libraries to" $(WICKR_DTOP_PLATFORM_OSX_REL)
	mkdir -p $(WICKR_DTOP_PLATFORM_OSX_REL)
	@cd $(WICKR_SDK); cp products/release/libwickrcore.a $(WICKR_DTOP_PLATFORM_OSX_REL)


##########################################################
# Windows 32 build
WICKR_SDK_WIN32 = $(WICKR_SDK)/build-win32
WICKR_SDK_WIN32_DEB = $(WICKR_SDK)/build-win32/.debug
WICKR_SDK_WIN32_REL = $(WICKR_SDK)/build-win32/.release

win32:
	cd $(WICKR_SDK); make win32

win32.release:
	cd $(WICKR_SDK); make win32.release

# WIN32 installation definitions
WICKR_DTOP_PLATFORM_WIN32_DEB = "$(subst /,\\,$(DIR)/platforms/win/lib32/debug)"
WICKR_DTOP_PLATFORM_WIN32_REL = "$(subst /,\\,$(DIR)/platforms/win/lib32/release)"
WICKR_DTOP_PLATFORM_WIN32_LIB = "$(subst /,\\,$(DIR)/platforms/win/lib32)"

win32.install: win32 install.headers
	@echo "Moving WIN32 (debug) libraries to" $(WICKR_DTOP_PLATFORM_WIN32_DEB)
	mkdir -p $(WICKR_DTOP_PLATFORM_WIN32_DEB)
	@cd $(WICKR_SDK); cp Debug/WickrCore.dll $(WICKR_DTOP_PLATFORM_WIN32_DEB)
	@cd $(WICKR_SDK); cp Debug/WickrCore.lib $(WICKR_DTOP_PLATFORM_WIN32_DEB)
	@cd $(WICKR_SDK)/localRepo/wickr-crypto/win32/lib/Release; cp libeay32.lib ssleay32.lib $(WICKR_DTOP_PLATFORM_WIN32_LIB)

win32.release.install: win32.release install.headers
	@echo "Moving WIN32 (release) libraries to" $(WICKR_DTOP_PLATFORM_WIN32_REL)
	mkdir -p $(WICKR_DTOP_PLATFORM_WIN32_REL)
	@cd $(WICKR_SDK); cp Release/WickrCore.dll $(WICKR_DTOP_PLATFORM_WIN32_REL)
	@cd $(WICKR_SDK); cp Release/WickrCore.lib $(WICKR_DTOP_PLATFORM_WIN32_REL)
	@cd $(WICKR_SDK)/localRepo/wickr-crypto/win32/lib/Release; cp libeay32.lib ssleay32.lib $(WICKR_DTOP_PLATFORM_WIN32_LIB)

win32.clean:
	cd $(WICKR_SDK); make win32.clean
	# TODO: Remove the libraries

##########################################################
# Linux build
WICKR_SDK_LINUX = $(WICKR_SDK)/build-unix
WICKR_SDK_LINUX_DEB = $(WICKR_SDK)/build-unix/.debug
WICKR_SDK_LINUX_REL = $(WICKR_SDK)/build-unix/.release

linux:
	cd $(WICKR_SDK); make linux

linux.release:
	cd $(WICKR_SDK); make linux.release

linux.clean:
	cd $(WICKR_SDK); make linux.clean

# Linux installation definitions
WICKR_DTOP_PLATFORM_LINUX_DEB = $(DIR)/platforms/linux/generic-64/debug
WICKR_DTOP_PLATFORM_LINUX_REL = $(DIR)/platforms/linux/generic-64/release

linux.install: linux install.headers
	@echo "Moving Linux (debug) libraries to" $(WICKR_DTOP_PLATFORM_LINUX_DEB)
	mkdir -p $(WICKR_DTOP_PLATFORM_LINUX_DEB)
	@cd $(WICKR_SDK); cp lib-lin64/libWickrCoreCdbg.a $(WICKR_DTOP_PLATFORM_LINUX_DEB)/libWickrCoreC.a

linux.release.install: linux.release install.headers
	@echo "Moving Linux (release) libraries to" $(WICKR_DTOP_PLATFORM_LINUX_REL)
	mkdir -p $(WICKR_DTOP_PLATFORM_LINUX_REL)
	@cd $(WICKR_SDK); cp lib-lin64/libWickrCoreC.a $(WICKR_DTOP_PLATFORM_LINUX_REL)/libWickrCoreC.a


##########################################################
all: $(WICKR_SDK)/wickr-sdk.pro
	@echo done!
