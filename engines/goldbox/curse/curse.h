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

#ifndef GOLDBOX_CURSE_CURSE_H
#define GOLDBOX_CURSE_CURSE_H

#include "goldbox/engine.h"

namespace Goldbox {
namespace Curse {

/**
 * Minimal stand-in for Curse of the Azure Bonds.
 *
 * Exists solely to prove that GAMETYPE_CURSE can be detected and routed
 * through GameFactory to a concrete Goldbox::Engine subclass end-to-end.
 * Real ECL/DAX wiring should replace the stub bodies as game data support
 * is added, mirroring the Poolrad engine's structure.
 */
class CurseEngine : public Goldbox::Engine {
protected:
	GUI::Debugger *getConsole() override;

public:
	CurseEngine(OSystem *syst, const GoldboxGameDescription *gameDesc);
	~CurseEngine() override;
};

// Referencing this symbol from metaengine.cpp forces the archive member
// containing this file's GameFactory::Registrar to be linked into static
// builds (module.mk builds each engine as a real .a, so otherwise nothing
// would pull in an object whose only effect is a static initializer).
extern int kForceLinkCurse;

} // End of namespace Curse
} // End of namespace Goldbox

#endif // GOLDBOX_CURSE_CURSE_H
