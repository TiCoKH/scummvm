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
 *
 * This file is dual-licensed.
 * In addition to the GPLv3 license mentioned above, this code is also
 * licensed under LGPL 2.1. See LICENSES/COPYING.LGPL file for the
 * full text of the license.
 *
 */

#ifndef GOB_CHEATER_H
#define GOB_CHEATER_H

namespace GUI {
	class Debugger;
}

namespace Gob {

namespace Geisha {
	class Diving;
	class Penetration;
}

class GobEngine;

class Cheater {
public:
	Cheater(GobEngine *vm);
	virtual ~Cheater();

	virtual bool cheat(GUI::Debugger &console) = 0;

protected:
	GobEngine *_vm;
};

class Cheater_Geisha : public Cheater {
public:
	Cheater_Geisha(GobEngine *vm, Geisha::Diving *diving, Geisha::Penetration *penetration);
	~Cheater_Geisha() override;

	bool cheat(GUI::Debugger &console) override;

private:
	Geisha::Diving      *_diving;
	Geisha::Penetration *_penetration;
};

} // End of namespace Gob

#endif // GOB_CHEATER_H
