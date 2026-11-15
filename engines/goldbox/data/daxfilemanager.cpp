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
#include "goldbox/data/daxresourcefile.h"
#include "goldbox/data/format/dax_format.h"

namespace Goldbox {
namespace Data {

DaxFileManager::DaxFileManager(Common::Platform platform) :
    _format(DaxFormat::create(platform)),
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
    _containerItem(ContentType::ITEM),
    _containerPic(ContentType::EGAPIC),
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

void DaxFileManager::loadFamily(const char *pattern, uint16 dosIndices) {
    Common::Array<Common::Path> files;
    _format->familyFiles(pattern, dosIndices, files);
    loadFromFiles(files);
}

void DaxFileManager::loadFile(const Common::Path &path) {
    Common::String filename = path.toString(Common::Path::kNativeSeparator);
    filename.toUppercase();

    DaxResourceFile file;
    if (!file.open(path, *_format)) {
        warning("DaxFileManager::loadFile: Failed to open file: %s",
                path.toString(Common::Path::kNativeSeparator).c_str());
        return;
    }

    debug(1, "DaxFileManager: Loading file '%s'", filename.c_str());

    // Determine which container to load into based on filename
    DaxBlockContainer *container = nullptr;
    if (filename.contains("8X8D")) {
        debug(1, "  - Detected 8X8D container");
        container = &_container8x8d;
    } else if (filename.contains("BACPAC")) {
        debug(1, "  - Detected BACPAC container");
        container = &_containerBacpac;
    } else if (filename.contains("DUNGCOM")) {
        debug(1, "  - Detected DUNGCOM container");
        container = &_containerDungcom;
    } else if (filename.contains("RANDCOM")) {
        debug(1, "  - Detected RANDCOM container");
        container = &_containerRandcom;
    } else if (filename.contains("SQRPACI")) {
        debug(1, "  - Detected SQRPACI container");
        container = &_containerSqrpaci;
    } else if (filename.contains("CBODY")) {
        debug(1, "  - Detected CBODY container");
        container = &_containerCBody;
    } else if (filename.contains("BODY")) {
        debug(1, "  - Detected BODY container");
        container = &_containerBody;
    } else if (filename.contains("CHEAD")) {
        debug(1, "  - Detected CHEAD container");
        container = &_containerCHead;
    } else if (filename.contains("COMSPR")) {
        debug(1, "  - Detected COMSPR container (IMPORTANT!)");
        container = &_containerComSpr;
    } else if (filename.contains("CPIC")) {
        debug(1, "  - Detected CPIC container");
        container = &_containerCPic;
    } else if (filename.contains("ECL")) {
        debug(1, "  - Detected ECL container");
        container = &_containerEcl;
    } else if (filename.contains("GEO")) {
        debug(1, "  - Detected GEO container");
        container = &_containerGeo;
    } else if (filename.contains("HEAD")) {
        debug(1, "  - Detected HEAD container");
        container = &_containerHead;
    } else if (filename.contains("MON") && filename.contains("CHA")) {
        debug(1, "  - Detected MONCHA container");
        container = &_containerMonCha;
    } else if (filename.contains("MON") && filename.contains("ITM")) {
        debug(1, "  - Detected MONITM container");
        container = &_containerMonItm;
    } else if (filename.contains("ITEM")) {
        debug(1, "  - Detected ITEM container");
        container = &_containerItem;
    } else if (filename.contains("MON") && filename.contains("SPC")) {
        debug(1, "  - Detected MONSPC container");
        container = &_containerMonSpc;
    } else if (filename.contains("TITLE")) {
        debug(1, "  - Detected TITLE container");
        container = &_containerTitle;
    } else if (filename.contains("SPRIT")) {
        debug(1, "  - Detected SPRIT container");
        container = &_containerSprit;
    } else if (filename.contains("WALLDEF")) {
        debug(1, "  - Detected WALLDEF container");
        container = &_containerWalldef;
    } else if (filename.contains("PIC")) {
        debug(1, "  - Detected PIC container");
        container = &_containerPic;
    } else if (filename.contains("WILDCOM")) {
        debug(1, "  - Detected WILDCOM container");
        container = &_containerWildcom;
    } else {
        debug(1, "  - No matching container for filename");
    }

    if (container) {
        container->loadFromFile(&file);
        _format->addAliases(path, *container);
    }

    file.close();
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
    _containerItem.clear();
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
