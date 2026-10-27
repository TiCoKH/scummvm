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

#include "goldbox/metaengine.h"

#include "common/config-manager.h"
#include "common/translation.h"
#include "engines/dialogs.h"
#include "goldbox/detection.h"
//#include "goldbox/keymapping.h"
#include "goldbox/game_factory.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/curse/curse.h"
#include "goldbox/engine.h"
#include "goldbox/sound/sound_driver.h"
#include "gui/ThemeEval.h"
#include "gui/widget.h"

namespace Goldbox {

namespace {
// Anchors that force each game's translation unit (and its
// GameFactory::Registrar static initializer) to be linked into static
// builds. Adding a new game means adding one term here plus its own
// self-registering .cpp file - the createInstance() dispatch below never
// needs to change.
volatile int gForceLinkGames =
	Goldbox::Poolrad::kForceLinkPoolrad +
	Goldbox::Curse::kForceLinkCurse;
} // End of anonymous namespace

static const ADExtraGuiOptionsMap optionsList[] = {
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
	AD_EXTRA_GUI_OPTIONS_TERMINATOR
};

/** Per-game sound selectors and the original save/load option. */
class GoldboxOptionsWidget : public GUI::OptionsContainerWidget {
public:
    GoldboxOptionsWidget(GUI::GuiObject *boss, const Common::String &name,
            const Common::String &domain);

    void load() override;
    bool save() override;
    bool hasKeys() override;
    void setEnabled(bool enabled) override;

protected:
    void defineLayout(GUI::ThemeEval &layouts,
        const Common::String &layoutName,
        const Common::String &overlayedLayout) const override;

private:
    GUI::RadiobuttonGroup _soundModeGroup;
    GUI::RadiobuttonGroup _soundVersionGroup;
    GUI::CheckboxWidget *_originalMenus;
};

GoldboxOptionsWidget::GoldboxOptionsWidget(GUI::GuiObject *boss,
        const Common::String &name, const Common::String &domain)
        : OptionsContainerWidget(boss, name, "GoldboxGameOptions", domain),
        _soundModeGroup(this), _soundVersionGroup(this) {
    _originalMenus = new GUI::CheckboxWidget(widgetsBoss(),
        _dialogLayout + ".original_menus", _("Use original save/load screens"),
        _("Use the original save/load screens instead of the ScummVM ones"));

    new GUI::StaticTextWidget(widgetsBoss(), _dialogLayout + ".sound_device",
        _("Sound device:"));
    new GUI::RadiobuttonWidget(widgetsBoss(), _dialogLayout + ".tandy",
        &_soundModeGroup, kSoundTandy, _("Tandy 3-voice"));
    new GUI::RadiobuttonWidget(widgetsBoss(), _dialogLayout + ".speaker",
        &_soundModeGroup, kSoundPCSpeaker, _("PC speaker"));
    new GUI::RadiobuttonWidget(widgetsBoss(), _dialogLayout + ".off",
        &_soundModeGroup, kSoundOff, _("Off"));

    new GUI::StaticTextWidget(widgetsBoss(), _dialogLayout + ".sound_version",
        _("Sound data version:"));
    new GUI::RadiobuttonWidget(widgetsBoss(), _dialogLayout + ".v13",
        &_soundVersionGroup, 13, _("v1.3"),
        _("Use the original v1.3 sounds from sound.dat"));
    new GUI::RadiobuttonWidget(widgetsBoss(), _dialogLayout + ".v10",
        &_soundVersionGroup, 10, _("v1.0"),
        _("Use the original v1.0 sounds from sound-v10.dat"));
}

void GoldboxOptionsWidget::load() {
    _originalMenus->setState(ConfMan.hasKey("original_menus", _domain) &&
        ConfMan.getBool("original_menus", _domain));
    const Common::String mode = ConfMan.get("sound_mode", _domain);
    _soundModeGroup.setValue(mode.equalsIgnoreCase("off") ? kSoundOff :
        mode.equalsIgnoreCase("speaker") ? kSoundPCSpeaker : kSoundTandy);
    const Common::String version = ConfMan.get("sound_version", _domain);
    _soundVersionGroup.setValue(version == "1.0" ? 10 : 13);
}

bool GoldboxOptionsWidget::save() {
    const int mode = _soundModeGroup.getValue();
    const char *modeName = mode == kSoundOff ? "off" :
        mode == kSoundPCSpeaker ? "speaker" : "tandy";
    const char *version = _soundVersionGroup.getValue() == 10 ? "1.0" : "1.3";
    const bool changed =
        ConfMan.get("sound_mode", _domain) != modeName ||
        ConfMan.get("sound_version", _domain) != version ||
        !ConfMan.hasKey("original_menus", _domain) ||
        ConfMan.getBool("original_menus", _domain) != _originalMenus->getState();

    ConfMan.set("sound_mode", modeName, _domain);
    ConfMan.set("sound_version", version, _domain);
    ConfMan.setBool("original_menus", _originalMenus->getState(), _domain);
    return changed;
}

bool GoldboxOptionsWidget::hasKeys() {
    return ConfMan.hasKey("sound_mode", _domain) ||
        ConfMan.hasKey("sound_version", _domain) ||
        ConfMan.hasKey("original_menus", _domain);
}

void GoldboxOptionsWidget::setEnabled(bool enabled) {
    _originalMenus->setEnabled(enabled);
    _soundModeGroup.setEnabled(enabled);
    _soundVersionGroup.setEnabled(enabled);
}

void GoldboxOptionsWidget::defineLayout(GUI::ThemeEval &layouts,
        const Common::String &layoutName,
        const Common::String &overlayedLayout) const {
    layouts.addDialog(layoutName, overlayedLayout);
    layouts.addLayout(GUI::ThemeLayout::kLayoutVertical)
        .addPadding(16, 0, 0, 0);
    layouts.addWidget("original_menus", "Checkbox");
    layouts.addSpace(8);
    layouts.addWidget("sound_device", "OptionsLabel");
    layouts.addWidget("tandy", "Radiobutton");
    layouts.addWidget("speaker", "Radiobutton");
    layouts.addWidget("off", "Radiobutton");
    layouts.addSpace(8);
    layouts.addWidget("sound_version", "OptionsLabel");
    layouts.addWidget("v13", "Radiobutton");
    layouts.addWidget("v10", "Radiobutton");
    layouts.closeLayout().closeDialog();
}

} // End of namespace Goldbox

const char *GoldboxMetaEngine::getName() const {
	return "goldbox";
}

const ADExtraGuiOptionsMap *GoldboxMetaEngine::getAdvancedExtraGuiOptions() const {
	return Goldbox::optionsList;
}

GUI::OptionsContainerWidget *GoldboxMetaEngine::buildEngineOptionsWidget(
        GUI::GuiObject *boss, const Common::String &name,
        const Common::String &target) const {
    if (!target.empty() && Common::checkGameGUIOption(
            GAMEOPTION_SOUND_MODE, ConfMan.get("guioptions", target)))
        return new Goldbox::GoldboxOptionsWidget(boss, name, target);
    return MetaEngine::buildEngineOptionsWidget(boss, name, target);
}

Common::Error GoldboxMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	const auto *gd = (Goldbox::GoldboxGameDescription *)desc;

	Goldbox::Engine *goldboxEngine = Goldbox::GameFactory::create(gd->gameType, syst, gd);
	if (!goldboxEngine)
		error("Unknown or unsupported game type %d", gd->gameType);

	*engine = goldboxEngine;
	return Common::kNoError;
}

bool GoldboxMetaEngine::hasFeature(MetaEngineFeature f) const {
	return checkExtendedSaves(f) ||
		(f == kSupportsLoadingDuringStartup);
}
/*
Common::KeymapArray GoldboxMetaEngine::initKeymaps(const char *target) const {
	return Goldbox::Keymapping::initKeymaps();
}
*/
#if PLUGIN_ENABLED_DYNAMIC(GOLDBOX)
REGISTER_PLUGIN_DYNAMIC(GOLDBOX, PLUGIN_TYPE_ENGINE, GoldboxMetaEngine);
#else
REGISTER_PLUGIN_STATIC(GOLDBOX, PLUGIN_TYPE_ENGINE, GoldboxMetaEngine);
#endif
