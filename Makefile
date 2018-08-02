WICKR_SDK = wickr-sdk
WICKR_INTEGRATIONS = wickr-integrations
LIBS_NZMQT = libs/nzmqt
NZMQT_BRANCH = master
LIBS_CPPZMQ = libs/third_party/cppzmq
LIBS_LIBZMQ = libs/third_party/libzmq
CLIENTAPI_LIB = clients/libs/WickrIOClientAPI
CLIENTAPI_CPPTEST = integrations/cpp/cpp_test
NODEJS_ADDON = integrations/nodejs/addon
LOCALREPO = localRepo/$(WICKR_SDK)
SDK_BRANCH = v4.41
INTEGRATIONS_BRANCH = alpha

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

update: sdk.update integrations.update nzmqt.update

clean: osx.clean
	rm -rf $(HEADERDIR)

$(LIBS_LIBZMQ)/CMakeLists.txt:
	@echo $(HEADER_START)
	@echo "Starting to install libs/third_party/libzmq from GIT"
	git submodule init libs/third_party/libzmq
	git submodule update --recursive libs/third_party/libzmq
	@echo "Finished to install libs/third_party/libzmq from GIT"
	@echo $(HEADER_END)

$(LIBS_CPPZMQ)/CMakeLists.txt:
	@echo $(HEADER_START)
	@echo "Starting to install libs/third_party/cppzmq from GIT"
	git submodule init libs/third_party/cppzmq
	git submodule update --recursive libs/third_party/cppzmq
	@echo "Finished to install libs/third_party/cppzmq from GIT"
	@echo $(HEADER_END)

$(LIBS_NZMQT)/nzmqt.pri:
	@echo $(HEADER_START)
	@echo "Starting to install libs/nzmqt from GIT"
	git submodule init libs/nzmqt
	git submodule update --recursive libs/nzmqt
	cd $(LIBS_NZMQT); git fetch --all; git checkout $(NZMQT_BRANCH); git pull
	@echo "Finished to install libs/nzmqt from GIT"
	@echo $(HEADER_END)

nzmqt.update:
	@echo $(HEADER_START)
	@echo "Update the libs/nzmqt submodule"
	@echo "Starting to update libs/nzmqt"
	cd $(LIBS_NZMQT); git fetch --all; git checkout $(NZMQT_BRANCH); git pull
	@echo $(HEADER_END)

$(WICKR_SDK)/wickr-sdk.pro:
	@echo $(HEADER_START)
	@echo "Starting to install wickr-sdk from GIT"
	git status
	git submodule init wickr-sdk
	git submodule update --recursive wickr-sdk
	cd $(WICKR_SDK); git fetch --all; git checkout $(SDK_BRANCH); git pull
	cd $(WICKR_SDK); make
	@echo "Finished to install wickr-sdk from GIT"
	@echo $(HEADER_END)

sdk.update:
	@echo $(HEADER_START)
	@echo "Update the SDK submodule"
	@echo "Starting to update wickr-sdk"
	cd $(WICKR_SDK); git fetch --all; git checkout $(SDK_BRANCH); git pull; make; make update
	@echo $(HEADER_END)

$(WICKR_INTEGRATIONS)/compress.sh:
	@echo $(HEADER_START)
	@echo "Starting to install wickr-integrations from GIT"
	git status
	git submodule init wickr-integrations
	git submodule update --recursive wickr-integrations
	cd $(WICKR_INTEGRATIONS); git fetch --all; git checkout $(INTEGRATIONS_BRANCH); git pull
	@echo "Finished to install wickr-integrations from GIT"
	@echo $(HEADER_END)

integrations.update:
	@echo $(HEADER_START)
	@echo "Update the Integrations submodule"
	@echo "Starting to update wickr-integrations"
	cd $(WICKR_INTEGRATIONS); git checkout master; git fetch --all; git checkout $(INTEGRATIONS_BRANCH); git pull
	@echo $(HEADER_END)

##########################################################
# OSX build

osx:
	cd $(WICKR_SDK); make osx

osx.release:
	cd $(WICKR_SDK); make osx.release

osx.clean:
	cd $(WICKR_SDK); make osx.clean

osx.install: osx
	@cd $(WICKR_SDK); make osx.install

osx.release.install: osx.release
	@cd $(WICKR_SDK); make osx.release.install


##########################################################
# Windows 32 build

win32:
	cd $(WICKR_SDK); make win32

win32.release:
	cd $(WICKR_SDK); make win32.release

win32.install: win32
	@cd $(WICKR_SDK); make win32.install

win32.release.install: win32.release
	@cd $(WICKR_SDK); make win32.release.install

win32.clean:
	cd $(WICKR_SDK); make win32.clean
	# TODO: Remove the libraries

##########################################################
# Linux build

linux:
	cd $(LIBS_LIBZMQ); mkdir -p build; cd build; cmake ..; make -j4
	cd $(LIBS_CPPZMQ); mkdir -p build; cd build; cmake ..; make -j4
	cd $(CLIENTAPI_LIB); mkdir -p build; cd build; cmake ..; make
	cd $(CLIENTAPI_CPPTEST); mkdir -p build; cd build; cmake ..; make
	cd $(NODEJS_ADDON); npm install nan --save; cmake-js
	cd $(WICKR_SDK); make linux

linux.release:
	cd $(LIBS_LIBZMQ); mkdir -p build; cd build; cmake ..; make -j4
	cd $(LIBS_CPPZMQ); mkdir -p build; cd build; cmake ..; make -j4
	cd $(CLIENTAPI_LIB); mkdir -p build; cd build; cmake ..; make
	cd $(CLIENTAPI_CPPTEST); mkdir -p build; cd build; cmake ..; make
	cd $(NODEJS_ADDON); npm install nan --save; cmake-js
	cd $(WICKR_SDK); make linux.release

linux.clean:
	cd $(LIBS_LIBZMQ); rm -rf build
	cd $(LIBS_CPPZMQ); rm -rf build
	cd $(CLIENTAPI_LIB); rm -rf build
	cd $(CLIENTAPI_CPPTEST); rm -rf build
	cd $(NODEJS_ADDON); rm -rf build node_modules
	cd $(WICKR_SDK); make linux.clean

linux.install: linux
	@cd $(WICKR_SDK); make linux.install

linux.release.install: linux.release
	@cd $(WICKR_SDK); make linux.release.install


##########################################################
all: $(WICKR_SDK)/wickr-sdk.pro $(LIBS_LIBZMQ)/CMakeLists.txt $(LIBS_CPPZMQ)/CMakeLists.txt $(LIBS_NZMQT)/nzmqt.pri $(WICKR_INTEGRATIONS)/compress.sh
	@echo done!
