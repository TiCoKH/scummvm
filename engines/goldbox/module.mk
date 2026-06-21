MODULE := engines/goldbox

MODULE_OBJS = \
	engine.o \
	console.o \
	events.o \
	keymapping.o \
	messages.o \
	metaengine.o \
	runtime/runtime_exchange.o \
	runtime/runtime_geo.o \
	runtime/runtime_time.o \
	runtime/treasure_pool.o \
	vm_interface.o \
	ecl/ecl_decoder.o \
	ecl/ecl_memory.o \
	ecl/runtime_layout.o \
	ecl/ecl_syscall_impl.o \
	ecl/ecl_vm.o \
	ecl/opcode_handlers.o \
	ecl/opcode_table.o \
	core/array.o \
	core/file.o \
	core/menu_item.o \
	core/vm_bank.o \
	core/vm_layout.o \
	data/player_character.o \
	data/adnd_character.o \
	data/daxblock.o \
	data/daxblockcontainer.o \
	data/daxfilemanager.o \
	data/daxheadercontainer.o \
	data/daxresourcefile.o \
	data/pascal_string_buffer.o \
	data/strings.o \
	data/strings_data.o \
	data/effects/character_effects.o \
	data/effects/effect_handler_base.o \
	data/effects/effect_runtime.o \
	data/effects/effect_system.o \
	data/items/base_items.o \
	data/items/character_item.o \
	data/items/character_inventory.o \
	data/rules/rules_types.o \
	data/rules/rules_poolrad.o \
	data/spells/spell_book.o \
	spells/spell_metadata.o \
	spells/spell_casting.o \
	spells/spell_handlers.o \
	spells/spell_registry.o \
	spells/spell_targeter.o \
	gfx/dax_font.o \
	gfx/dax_tile.o \
	gfx/dax_renderer.o \
	gfx/dax_anim_decoder.o \
	gfx/first_person_renderer.o \
	gfx/area_map_cache.o \
	gfx/encounter_sprite_cache.o \
	gfx/picture_display_cache.o \
	gfx/pic.o \
	gfx/viewport_background.o \
	gfx/walldef_surface_builder.o \
	gfx/icon.o \
	gfx/icon_manager.o \
	gfx/combat_renderer.o \
	gfx/surface.o \
	sound/sound_driver.o \
	poolrad/poolrad.o \
	poolrad/poolrad_runtime_exchange.o \
	poolrad/effect_handler.o \
	poolrad/console.o \
	poolrad/ecl/poolrad_engine_host_impl.o \
	poolrad/ecl/poolrad_opcode_handlers.o \
	poolrad/data/poolrad_character.o \
	poolrad/data/legacy_save_utils.o \
	poolrad/data/poolrad_vm_layout.o \
	poolrad/gfx/surface.o \
	poolrad/views/view.o \
	poolrad/views/title_view.o \
	poolrad/views/credits_view.o \
	poolrad/views/codewheel_view.o \
	poolrad/views/mainmenu_view.o \
	poolrad/views/create_character_view.o \
	poolrad/views/add_character_view.o \
	poolrad/views/view_character_view.o \
	poolrad/views/in_game_view.o \
	poolrad/views/dialogs/dialog.o \
	poolrad/views/dialogs/door_dialog.o \
	poolrad/views/dialogs/spell_book_dialog.o \
	poolrad/views/dialogs/horizontal_input.o \
	poolrad/views/dialogs/horizontal_menu.o \
	poolrad/views/dialogs/horizontal_yesno.o \
	poolrad/views/dialogs/party_list.o \
	poolrad/views/dialogs/party_selector.o \
	poolrad/views/dialogs/vertical_menu.o \
	poolrad/views/dialogs/prompt_message.o \
	poolrad/views/dialogs/load_save_dialog.o \
	poolrad/views/dialogs/items_menu.o \
	poolrad/views/dialogs/spells_menu.o \
	poolrad/views/dialogs/character_profile.o \
	poolrad/views/dialogs/portrait_display.o \
	poolrad/views/dialogs/in_game_main_screen_dialog.o \
	poolrad/views/dialogs/in_game_state_area_dialog.o \
	poolrad/views/dialogs/in_game_panel_dialog.o \
	poolrad/views/dialogs/text_box_dialog.o \
	poolrad/views/dialogs/in_game_menu_dialog.o \
	poolrad/views/dialogs/camp_menu_dialog.o \
	poolrad/views/dialogs/shop_base_dialog.o \
	poolrad/views/dialogs/temple_dialog.o \
	poolrad/views/dialogs/store_dialog.o \
	poolrad/views/dialogs/treasure_dialog.o \
	poolrad/views/dialogs/set_portrait.o \
	poolrad/views/dialogs/set_icon.o



# This module can be built as a plugin
ifeq ($(ENABLE_GOLDBOX), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
