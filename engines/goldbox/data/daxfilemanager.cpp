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

#include "common/util.h"
#include "goldbox/data/daxfilemanager.h"
#include "goldbox/core/file.h"

namespace Goldbox {
namespace Data {

DaxFileManager::DaxFileManager() :
    _container8x8d(ContentType::TILE),
    _containerBacpac(ContentType::TILE),
    _containerDungcom(ContentType::TILE),
    _containerRandcom(ContentType::TILE),
    _containerSqrpaci(ContentType::TILE),
    _containerBody(ContentType::BODY),
    _containerCBody(ContentType::CBODY),
    _containerCHead(ContentType::CHEAD),
    _containerComSpr(ContentType::COMSPR),
    _containerEcl(ContentType::ECL),
    _containerGeo(ContentType::GEO),
    _containerHead(ContentType::HEAD),
    _containerMonCha(ContentType::MONCHA),
    _containerMonItm(ContentType::UNKNOWN),  // MON*ITM is a separate category
    _containerMonSpc(ContentType::UNKNOWN),  // MON*SPC is a separate category
    _containerPic(ContentType::PIC),
    _containerCPic(ContentType::PIC),        // CPIC is PIC content
    _containerSprit(ContentType::SPRIT),
    _containerTitle(ContentType::TITLE),
    _containerWalldef(ContentType::WALLDEF) {
}

DaxFileManager::~DaxFileManager() {
    clear();
}

void DaxFileManager::loadFromFiles(const Common::Array<Common::Path> &paths) {
    for (const auto &path : paths) {
        loadFile(path);
    }
}

void DaxFileManager::loadFile(const Common::Path &path) {
    Common::String filename = path.toString(Common::Path::kNativeSeparator);
    filename.toUppercase();

    File *file = new File(path);
    if (!file->isOpen()) {
        warning("DaxFileManager::loadFile: Failed to open file: %s",
                path.toString(Common::Path::kNativeSeparator).c_str());
        delete file;
        return;
    }

    // Determine which container to load into based on filename
    if (filename.contains("8X8D")) {
        _container8x8d.loadFromFile(file);
    } else if (filename.contains("BACPAC")) {
        _containerBacpac.loadFromFile(file);
    } else if (filename.contains("DUNGCOM")) {
        _containerDungcom.loadFromFile(file);
    } else if (filename.contains("RANDCOM")) {
        _containerRandcom.loadFromFile(file);
    } else if (filename.contains("SQRPACI")) {
        _containerSqrpaci.loadFromFile(file);
    } else if (filename.contains("CBODY")) {
        _containerCBody.loadFromFile(file);
    } else if (filename.contains("BODY")) {
        _containerBody.loadFromFile(file);
    } else if (filename.contains("CHEAD")) {
        _containerCHead.loadFromFile(file);
    } else if (filename.contains("COMSPR")) {
        _containerComSpr.loadFromFile(file);
    } else if (filename.contains("CPIC")) {
        _containerCPic.loadFromFile(file);
    } else if (filename.contains("ECL")) {
        _containerEcl.loadFromFile(file);
    } else if (filename.contains("GEO")) {
        _containerGeo.loadFromFile(file);
    } else if (filename.contains("HEAD")) {
        _containerHead.loadFromFile(file);
    } else if (filename.contains("MON") && filename.contains("CHA")) {
        _containerMonCha.loadFromFile(file);
    } else if (filename.contains("MON") && filename.contains("ITM")) {
        _containerMonItm.loadFromFile(file);
    } else if (filename.contains("MON") && filename.contains("SPC")) {
        _containerMonSpc.loadFromFile(file);
    } else if (filename.contains("SPRIT")) {
        _containerSprit.loadFromFile(file);
    } else if (filename.contains("WALLDEF")) {
        _containerWalldef.loadFromFile(file);
    } else if (filename.contains("PIC")) {
        _containerPic.loadFromFile(file);
    } else if (filename.contains("TITLE")) {
        _containerTitle.loadFromFile(file);
    }

    // Close file only if it was successfully opened
    if (file->isOpen()) {
        file->close();
    }
    delete file;
}

void DaxFileManager::clear() {
    _container8x8d.clear();
    _containerBacpac.clear();
    _containerDungcom.clear();
    _containerRandcom.clear();
    _containerSqrpaci.clear();
    _containerBody.clear();
    _containerCBody.clear();
    _containerCHead.clear();
    _containerComSpr.clear();
    _containerEcl.clear();
    _containerGeo.clear();
    _containerHead.clear();
    _containerMonCha.clear();
    _containerMonItm.clear();
    _containerMonSpc.clear();
    _containerPic.clear();
    _containerCPic.clear();
    _containerSprit.clear();
    _containerTitle.clear();
    _containerWalldef.clear();
}

} // namespace Data
} // namespace Goldbox
