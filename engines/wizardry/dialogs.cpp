/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/config-manager.h"
#include "common/translation.h"
#include "gui/gui-manager.h"

#include "wizardry/detection.h"
#include "wizardry/dialogs.h"

namespace Wizardry {

const ADExtraGuiOptionsMap optionsList[] = {
	{
		GAMEOPTION_ORIGINAL_SAVELOAD,
		{
			_s("Use original save/load screens"),
			_s("Use the original save/load screens instead of the ScummVM ones"),
			"original_menus",
			false,
			0,
			0
		}
	},
	{
		GAMEOPTION_HIRES,
		{
			_s("Use 400-line graphics"),
			_s("Use the 640x400 character set and pictures instead of the 640x200 ones"),
			"hires",
			true,
			0,
			0
		}
	},
	AD_EXTRA_GUI_OPTIONS_TERMINATOR
};

const PopUpOptionsMap popUpOptionsList[] = {
	{
		GAMEOPTION_CGA_PALETTE,
		_s("CGA palette:"),
		_s("The colours of the four-colour CGA graphics, all at high intensity. "
			"The original showed cyan, red and white (palette 3, video mode 5), "
			"or cyan, magenta and white when Ctrl was held at start-up. "
			"No effect in the black-and-white render mode."),
		"cga_palette",
		kCgaPalette3,
		{
			{ _s("Palette 3: cyan, red, white"), kCgaPalette3 },
			{ _s("Palette 1: cyan, magenta, white"), kCgaPalette1 },
			{ _s("Palette 0: green, red, yellow"), kCgaPalette0 },
			POPUP_OPTIONS_ITEMS_TERMINATOR
		}
	},
	{
		GAMEOPTION_TEXT_LANGUAGE,
		_s("Text:"),
		_s("The language of the game's text. The original asks at every start; "
			"names typed in one are shown as they were typed in the other. "
			"Kanji needs FONT.ROM or FONT.BMP of a PC-98, or ScummVM's SJIS.FNT; "
			"without one, kana is shown."),
		"text_language",
		kTextEnglish,
		{
			{ _s("English"), kTextEnglish },
			{ _s("Hiragana/Katakana"), kTextKana },
			{ _s("Kanji"), kTextKanji },
			POPUP_OPTIONS_ITEMS_TERMINATOR
		}
	},
	POPUP_OPTIONS_TERMINATOR
};

OptionsWidget::OptionsWidget(GuiObject *boss, const Common::String &name, const Common::String &domain) :
		OptionsContainerWidget(boss, name, "WizardryGameOptionsDialog", domain) {
	_guiOptions = ConfMan.get("guioptions", domain);

	for (const ADExtraGuiOptionsMap *entry = optionsList; entry->guioFlag; ++entry) {
		if (checkGameGUIOption(entry->guioFlag, _guiOptions))
			_checkboxes[entry->option.configOption] = new GUI::CheckboxWidget(widgetsBoss(),
				_dialogLayout + "." + entry->option.configOption, _(entry->option.label), _(entry->option.tooltip));
	}

	for (const PopUpOptionsMap *entry = popUpOptionsList; entry->guioFlag; ++entry) {
		if (!checkGameGUIOption(entry->guioFlag, _guiOptions))
			continue;

		GUI::StaticTextWidget *text = new GUI::StaticTextWidget(widgetsBoss(),
			_dialogLayout + "." + entry->configOption + "_desc", _(entry->label), _(entry->tooltip));
		text->setAlign(Graphics::kTextAlignRight);

		GUI::PopUpWidget *popUp = new GUI::PopUpWidget(widgetsBoss(), _dialogLayout + "." + entry->configOption);
		for (uint i = 0; entry->items[i].label; ++i)
			popUp->appendEntry(_(entry->items[i].label), entry->items[i].configValue);
		_popUps[entry->configOption] = popUp;
	}
}

void OptionsWidget::defineLayout(GUI::ThemeEval &layouts, const Common::String &layoutName,
		const Common::String &overlayedLayout) const {
	layouts.addDialog(layoutName, overlayedLayout);
	layouts.addLayout(GUI::ThemeLayout::kLayoutVertical).addPadding(0, 0, 0, 0);

	for (const ADExtraGuiOptionsMap *entry = optionsList; entry->guioFlag; ++entry)
		layouts.addWidget(entry->option.configOption, "Checkbox");

	for (const PopUpOptionsMap *entry = popUpOptionsList; entry->guioFlag; ++entry) {
		layouts.addLayout(GUI::ThemeLayout::kLayoutHorizontal).addPadding(0, 0, 0, 0);
		layouts.addWidget(Common::String(entry->configOption) + "_desc", "OptionsLabel");
		layouts.addWidget(entry->configOption, "PopUp").closeLayout();
	}

	layouts.closeLayout().closeDialog();
}

void OptionsWidget::load() {
	for (const ADExtraGuiOptionsMap *entry = optionsList; entry->guioFlag; ++entry) {
		if (checkGameGUIOption(entry->guioFlag, _guiOptions))
			_checkboxes[entry->option.configOption]->setState(ConfMan.getBool(entry->option.configOption, _domain));
	}

	for (const PopUpOptionsMap *entry = popUpOptionsList; entry->guioFlag; ++entry) {
		if (checkGameGUIOption(entry->guioFlag, _guiOptions))
			_popUps[entry->configOption]->setSelectedTag(ConfMan.getInt(entry->configOption, _domain));
	}
}

bool OptionsWidget::save() {
	for (const ADExtraGuiOptionsMap *entry = optionsList; entry->guioFlag; ++entry) {
		if (checkGameGUIOption(entry->guioFlag, _guiOptions))
			ConfMan.setBool(entry->option.configOption, _checkboxes[entry->option.configOption]->getState(), _domain);
	}

	for (const PopUpOptionsMap *entry = popUpOptionsList; entry->guioFlag; ++entry) {
		if (checkGameGUIOption(entry->guioFlag, _guiOptions))
			ConfMan.setInt(entry->configOption, _popUps[entry->configOption]->getSelectedTag(), _domain);
	}

	return true;
}

} // End of namespace Wizardry
