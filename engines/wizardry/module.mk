MODULE := engines/wizardry

MODULE_OBJS = \
	wizardry.o \
	console.o \
	events.o \
	messages.o \
	metaengine.o \
	profile.o \
	data/globals.o \
	data/scenario.o \
	game/character.o \
	game/inn.o \
	game/items.o \
	game/party.o \
	game/shop.o \
	game/temple.o \
	game/training.o \
	libs/memory.o \
	libs/wizardry_archive.o \
	resources/cell_image.o \
	resources/charset.o \
	resources/dump.o \
	resources/files.o \
	resources/messages.o \
	resources/picture_file.o \
	resources/scenario_file.o \
	resources/title_image.o \
	gfx/cell_renderer.o \
	gfx/cell_screen.o \
	gfx/palette.o \
	gfx/surface.o \
	gfx/text_window.o \
	gfx/wizardry_font.o \
	views/view.o \
	views/castle.o \
	views/cell_view.o \
	views/gallery.o \
	views/title.o \
	views/town/castle.o \
	views/town/edge.o \
	views/town/inn.o \
	views/town/sheet.o \
	views/town/shop.o \
	views/town/tavern.o \
	views/town/temple.o \
	views/town/training.o \
	views/town/town_view.o \
	ui/menu.o \
	ui/town_screens.o

# This module can be built as a plugin
ifeq ($(ENABLE_WIZARDRY), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
