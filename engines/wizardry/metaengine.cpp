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

#include "common/translation.h"

#include "common/config-manager.h"
#include "wizardry/dialogs.h"
#include "wizardry/metaengine.h"
#include "wizardry/detection.h"
#include "wizardry/wizardry.h"


const char *WizardryMetaEngine::getName() const {
	return "wizardry";
}

void WizardryMetaEngine::registerDefaultSettings(const Common::String &target) const {
	for (const ADExtraGuiOptionsMap *entry = Wizardry::optionsList; entry->guioFlag; ++entry)
		ConfMan.registerDefault(entry->option.configOption, entry->option.defaultState);

	for (const Wizardry::PopUpOptionsMap *entry = Wizardry::popUpOptionsList; entry->guioFlag; ++entry)
		ConfMan.registerDefault(entry->configOption, entry->defaultState);
}

GUI::OptionsContainerWidget *WizardryMetaEngine::buildEngineOptionsWidget(GUI::GuiObject *boss,
		const Common::String &name, const Common::String &target) const {
	return new Wizardry::OptionsWidget(boss, name, target);
}

Common::Error WizardryMetaEngine::createInstance(OSystem *syst, Engine **engine,
		const Wizardry::WizardryGameDescription *desc) const {
	*engine = new Wizardry::WizardryEngine(syst, desc);
	return Common::kNoError;
}

bool WizardryMetaEngine::hasFeature(MetaEngineFeature f) const {
	return checkExtendedSaves(f) ||
		(f == kSupportsLoadingDuringStartup);
}

#if PLUGIN_ENABLED_DYNAMIC(WIZARDRY)
REGISTER_PLUGIN_DYNAMIC(WIZARDRY, PLUGIN_TYPE_ENGINE, WizardryMetaEngine);
#else
REGISTER_PLUGIN_STATIC(WIZARDRY, PLUGIN_TYPE_ENGINE, WizardryMetaEngine);
#endif
