MODULE := engines/goldbox

MODULE_OBJS = \
	engine.o \
	console.o \
	events.o \
	messages.o \
	metaengine.o \
	view.o \
	view1.o \
	data/daxblock.o \
	data/daxcache.o \
	data/daxheadercontainer.o

# This module can be built as a plugin
ifeq ($(ENABLE_GOLDBOX), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
