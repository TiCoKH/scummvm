MODULE := engines/goldbox

MODULE_OBJS = \
	engine.o \
	console.o \
	events.o \
	keymapping.o \
	messages.o \
	metaengine.o \
	vm_interface.o \
	core/array.o \
	core/file.o \
	core/menu_item.o \
	data/player_character.o \
	data/adnd_character.o \
	data/daxblock.o \
	data/daxheadercontainer.o \
	data/strings.o \
	data/strings_data.o \
	data/items/base_items.o \
	gfx/dax_font.o \
	gfx/dax_tile.o \
	gfx/pic.o \
	gfx/surface.o \
	poolrad/poolrad.o \
	poolrad/console.o \
	poolrad/data/poolrad_character.o \
	poolrad/views/view.o \
	poolrad/views/title_view.o \
	poolrad/views/credits_view.o \
	poolrad/views/codewheel_view.o \
	poolrad/views/mainmenu_view.o \
	poolrad/views/add_character_view.o \
	poolrad/views/dialogs/dialog.o \
	poolrad/views/dialogs/horizontal_input.o \
	poolrad/views/dialogs/horizontal_menu.o \
	poolrad/views/dialogs/vertical_menu.o 





# This module can be built as a plugin
ifeq ($(ENABLE_GOLDBOX), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
