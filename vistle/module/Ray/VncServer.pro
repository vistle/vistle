!include($$(COFRAMEWORKDIR)/mkspecs/config-first.pri):error(include of config-first.pri failed)
### don't modify anything before this line ###

TARGET = RRServer
PROJECT = General
TEMPLATE = opencoverplugin

NOT_AVAILABLE_ON=lovelock

CONFIG *= wnoerror
CONFIG *= ogl x11 jpegturbo
CONFIG *= cuda glew

DEFINES *= RR_COVISE

HEADERS = RRServer.h ReadBackCuda.h

SOURCES = RRServer.cpp

CUDA_SOURCES = ReadBackCuda.cu

EXTRASOURCES = *.h

### don't modify anything below this line ###
!include ($$(COFRAMEWORKDIR)/mkspecs/config-last.pri):error(include of config-last.pri failed)
