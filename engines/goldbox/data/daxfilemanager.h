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

#ifndef GOLDBOX_DATA_DAXFILEMANAGER_H
#define GOLDBOX_DATA_DAXFILEMANAGER_H

#include "common/array.h"
#include "common/path.h"
#include "common/ptr.h"
#include "goldbox/data/daxblockcontainer.h"

namespace Goldbox {
namespace Data {

/**
 * Manager for loading and organizing DAX files by content type.
 * Automatically categorizes DAX files and loads them into separate containers.
 * Handles file patterns like:
 *   8X8D1-8 (8x8 tiles), BACPAC/DUNGCOM/RANDCOM/SQRPACI (24x24 tiles),
 *   BODY1-8, CBODY, CHEAD, SPRIT1-8, WALLDEF1-8, etc.
 */
class DaxFileManager {
private:
    // Containers for each content type
    DaxBlockContainer _container8x8d;
    DaxBlockContainer _containerBacpac;
    DaxBlockContainer _containerDungcom;
    DaxBlockContainer _containerRandcom;
    DaxBlockContainer _containerSqrpaci;
    DaxBlockContainer _containerBody;
    DaxBlockContainer _containerCBody;
    DaxBlockContainer _containerCHead;
    DaxBlockContainer _containerComSpr;
    DaxBlockContainer _containerEcl;
    DaxBlockContainer _containerGeo;
    DaxBlockContainer _containerHead;
    DaxBlockContainer _containerMonCha;
    DaxBlockContainer _containerMonItm;
    DaxBlockContainer _containerMonSpc;
    DaxBlockContainer _containerPic;
    DaxBlockContainer _containerCPic;
    DaxBlockContainer _containerSprit;
    DaxBlockContainer _containerTitle;
    DaxBlockContainer _containerWalldef;

public:
    DaxFileManager();
    ~DaxFileManager();

    /**
     * Load DAX files from a directory or list of files.
     * Automatically categorizes them by filename pattern.
     * @param paths Array of file paths to load
     */
    void loadFromFiles(const Common::Array<Common::Path> &paths);

    /**
     * Load a single DAX file and add to appropriate container.
     * @param path Path to the DAX file
     */
    void loadFile(const Common::Path &path);

    // Accessors for each container type
    DaxBlockContainer &get8x8d() { return _container8x8d; }
    DaxBlockContainer &getBacpac() { return _containerBacpac; }
    DaxBlockContainer &getDungcom() { return _containerDungcom; }
    DaxBlockContainer &getRandcom() { return _containerRandcom; }
    DaxBlockContainer &getSqrpaci() { return _containerSqrpaci; }
    DaxBlockContainer &getBody() { return _containerBody; }
    DaxBlockContainer &getCBody() { return _containerCBody; }
    DaxBlockContainer &getCHead() { return _containerCHead; }
    DaxBlockContainer &getComSpr() { return _containerComSpr; }
    DaxBlockContainer &getEcl() { return _containerEcl; }
    DaxBlockContainer &getGeo() { return _containerGeo; }
    DaxBlockContainer &getHead() { return _containerHead; }
    DaxBlockContainer &getMonCha() { return _containerMonCha; }
    DaxBlockContainer &getMonItm() { return _containerMonItm; }
    DaxBlockContainer &getMonSpc() { return _containerMonSpc; }
    DaxBlockContainer &getPic() { return _containerPic; }
    DaxBlockContainer &getCPic() { return _containerCPic; }
    DaxBlockContainer &getSprit() { return _containerSprit; }
    DaxBlockContainer &getTitle() { return _containerTitle; }
    DaxBlockContainer &getWalldef() { return _containerWalldef; }

    // Const accessors
    const DaxBlockContainer &get8x8d() const { return _container8x8d; }
    const DaxBlockContainer &getBacpac() const { return _containerBacpac; }
    const DaxBlockContainer &getDungcom() const { return _containerDungcom; }
    const DaxBlockContainer &getRandcom() const { return _containerRandcom; }
    const DaxBlockContainer &getSqrpaci() const { return _containerSqrpaci; }
    const DaxBlockContainer &getBody() const { return _containerBody; }
    const DaxBlockContainer &getCBody() const { return _containerCBody; }
    const DaxBlockContainer &getCHead() const { return _containerCHead; }
    const DaxBlockContainer &getComSpr() const { return _containerComSpr; }
    const DaxBlockContainer &getEcl() const { return _containerEcl; }
    const DaxBlockContainer &getGeo() const { return _containerGeo; }
    const DaxBlockContainer &getHead() const { return _containerHead; }
    const DaxBlockContainer &getMonCha() const { return _containerMonCha; }
    const DaxBlockContainer &getMonItm() const { return _containerMonItm; }
    const DaxBlockContainer &getMonSpc() const { return _containerMonSpc; }
    const DaxBlockContainer &getPic() const { return _containerPic; }
    const DaxBlockContainer &getCPic() const { return _containerCPic; }
    const DaxBlockContainer &getSprit() const { return _containerSprit; }
    const DaxBlockContainer &getTitle() const { return _containerTitle; }
    const DaxBlockContainer &getWalldef() const { return _containerWalldef; }

    /**
     * Clear all containers.
     */
    void clear();
};

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_DAXFILEMANAGER_H
