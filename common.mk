# common.mk for the camera streamer application

# Standard QNX header - do not change
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

# --- Project Configuration ---

# 1. Define the name of the application.
#    This should match the name of your C source file.
NAME=camera_example1_callback

# 2. Define where the program will be installed on the target.
INSTALLDIR=usr/bin

# 3. The critical fix: Define the libraries this application needs.
#    We need both the camera API and the socket library.
LIBS+=camapi socket

# 4. Optional: Define PINFO for package information.
define PINFO
PINFO DESCRIPTION=Camera client application for streaming video
endef

# Standard QNX footer - do not change. This includes the build rules.
include $(MKFILES_ROOT)/qtargets.mk
