MODULE := engines/goldbox

MODULE_OBJS = \
	engine.o \
	console.o \
	events.o \
	keymapping.o \
	messages.o \
	metaengine.o \
	core/array.o \
	core/file.o \
	data/daxblock.o \
	data/daxheadercontainer.o \
	gfx/dax_font.o \
	gfx/dax_tile.o \
	gfx/pic.o \
	gfx/surface.o \
	poolrad/poolrad.o \
	poolrad/console.o \
	poolrad/views/title.o \
	poolrad/views/view.o \
	poolrad/views/dialogs/dialog.o \
	poolrad/views/dialogs/credits.o



# This module can be built as a plugin
ifeq ($(ENABLE_GOLDBOX), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
