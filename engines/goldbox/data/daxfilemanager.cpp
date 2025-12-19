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
#include "common/debug.h"
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
    _containerBody(ContentType::PIC),
    _containerCBody(ContentType::CTILE),
    _containerCHead(ContentType::CTILE),
    _containerComSpr(ContentType::CTILE),
    _containerEcl(ContentType::ECL),
    _containerGeo(ContentType::GEO),
    _containerHead(ContentType::PIC),
    _containerMonCha(ContentType::CHARACTER),
    _containerMonItm(ContentType::ITEM),
    _containerMonSpc(ContentType::SPELL),
    _containerPic(ContentType::PIC),
    _containerCPic(ContentType::CTILE),
    _containerSprit(ContentType::SPRIT),
    _containerTitle(ContentType::PIC),
    _containerWalldef(ContentType::WALLDEF),
    _containerWildcom(ContentType::TILE) {
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

    debug("DaxFileManager: Loading file '%s'", filename.c_str());

    // Determine which container to load into based on filename
    if (filename.contains("8X8D")) {
        debug("  - Detected 8X8D container");
        _container8x8d.loadFromFile(file);
    } else if (filename.contains("BACPAC")) {
        debug("  - Detected BACPAC container");
        _containerBacpac.loadFromFile(file);
    } else if (filename.contains("DUNGCOM")) {
        debug("  - Detected DUNGCOM container");
        _containerDungcom.loadFromFile(file);
    } else if (filename.contains("RANDCOM")) {
        debug("  - Detected RANDCOM container");
        _containerRandcom.loadFromFile(file);
    } else if (filename.contains("SQRPACI")) {
        debug("  - Detected SQRPACI container");
        _containerSqrpaci.loadFromFile(file);
    } else if (filename.contains("CBODY")) {
        debug("  - Detected CBODY container");
        _containerCBody.loadFromFile(file);
    } else if (filename.contains("BODY")) {
        debug("  - Detected BODY container");
        _containerBody.loadFromFile(file);
    } else if (filename.contains("CHEAD")) {
        debug("  - Detected CHEAD container");
        _containerCHead.loadFromFile(file);
    } else if (filename.contains("COMSPR")) {
        debug("  - Detected COMSPR container (IMPORTANT!)");
        _containerComSpr.loadFromFile(file);
        debug("  - COMSPR loaded, block count: %zu", _containerComSpr.getBlockCount());
    } else if (filename.contains("CPIC")) {
        debug("  - Detected CPIC container");
        _containerCPic.loadFromFile(file);
    } else if (filename.contains("ECL")) {
        debug("  - Detected ECL container");
        _containerEcl.loadFromFile(file);
    } else if (filename.contains("GEO")) {
        debug("  - Detected GEO container");
        _containerGeo.loadFromFile(file);
    } else if (filename.contains("HEAD")) {
        debug("  - Detected HEAD container");
        _containerHead.loadFromFile(file);
    } else if (filename.contains("MON") && filename.contains("CHA")) {
        debug("  - Detected MONCHA container");
        _containerMonCha.loadFromFile(file);
    } else if (filename.contains("MON") && filename.contains("ITM")) {
        debug("  - Detected MONITM container");
        _containerMonItm.loadFromFile(file);
    } else if (filename.contains("MON") && filename.contains("SPC")) {
        debug("  - Detected MONSPC container");
        _containerMonSpc.loadFromFile(file);
    } else if (filename.contains("TITLE")) {
        debug("  - Detected TITLE container");
        _containerTitle.loadFromFile(file);
    } else if (filename.contains("SPRIT")) {
        debug("  - Detected SPRIT container");
        _containerSprit.loadFromFile(file);
    } else if (filename.contains("WALLDEF")) {
        debug("  - Detected WALLDEF container");
        _containerWalldef.loadFromFile(file);
    } else if (filename.contains("PIC")) {
        debug("  - Detected PIC container");
        _containerPic.loadFromFile(file);
    } else if (filename.contains("WILDCOM")) {
        debug("  - Detected WILDCOM container");
        _containerWildcom.loadFromFile(file);
    } else {
        debug("  - No matching container for filename");
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
    _containerWildcom.clear();
}

} // namespace Data
} // namespace Goldbox
