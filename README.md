Currently only builds on OSX.

OSX Build Instructions:
1. Clone the repo
2. cd to the wickr-desktop-base directory
3. Pull in the submodules.  You can do this by running the "make" command
4. Run update to make sure the latest has been pulled down.  Run the "make update" command.
5. Build the submodules for OSX.  Run the "make osx" command.
6. Run command to install the submodules built files into the wickr-desktop-base target areas.  Run the "make install" command.
7. Build the library using Qt.  The top level project 'wickr-desktop-base.pro' should be used. Currently build with Qt 5.8.1

Seems like alot of steps for now.  But you should only have to run steps 1 through 6 for initial setup and then run them occasionally when the submodules are changes.

Links:
https://confluence.wickrlan.net:8443/pages/viewpage.action?pageId=9471405
