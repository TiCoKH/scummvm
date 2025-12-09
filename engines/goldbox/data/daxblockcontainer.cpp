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
#include "goldbox/data/daxblockcontainer.h"
#include "goldbox/core/file.h"

namespace Goldbox {
namespace Data {

DaxBlockContainer::DaxBlockContainer() : _contentType(ContentType::UNKNOWN) {
}

DaxBlockContainer::DaxBlockContainer(ContentType contentType) : _contentType(contentType) {
}

DaxBlockContainer::~DaxBlockContainer() {
    clear();
}

void DaxBlockContainer::loadFromFile(File *file) {
    if (!file || !file->isOpen()) {
        warning("DaxBlockContainer::loadFromFile: Invalid or closed file");
        return;
    }

    // Get all headers from the file
    const Common::Array<DaxHeader> &headers = file->headerContainer.getHeaders();

    // Load each block
    for (const auto &header : headers) {
        uint8 blockId = header.id;

        // Skip if block already exists (keep first occurrence)
        if (_blocks.contains(blockId)) {
            continue;
        }

        // Get block data from file
        file->seek(file->headerContainer.getFileDataOffset() + header.offset, SEEK_SET);
        Common::Array<uint8> daxData(header.compSize);
        file->read(daxData.data(), header.compSize);

        // Determine content type if not set
        ContentType blockContentType = _contentType;
        if (blockContentType == ContentType::UNKNOWN) {
            // Use the file's content type
            blockContentType = file->_ctype;
        }

        // Create the appropriate DaxBlock
        DaxBlock *daxBlock = DaxBlock::createDaxBlock(blockContentType);
        if (!daxBlock) {
            warning("DaxBlockContainer::loadFromFile: Unknown content type for block ID: %d", blockId);
            continue;
        }

        // Decode data if necessary
        if (header.rawSize <= 0) {
            daxBlock->_data = daxData;
        } else {
            Common::Array<uint8> decodeData(header.rawSize);
            file->decodeRLE(header.compSize, decodeData.data(), daxData.data());
            daxBlock->_data = decodeData;
        }

        daxBlock->blockId = blockId;
        daxBlock->adjust();

        // Store in container (first occurrence only)
        _blocks[blockId] = Common::SharedPtr<DaxBlock>(daxBlock);
    }
}

void DaxBlockContainer::loadFromFiles(const Common::Array<Common::Path> &filenames) {
    for (const auto &filename : filenames) {
        File *file = new File(filename);
        if (file->isOpen()) {
            loadFromFile(file);
            file->close();
            delete file;
        } else {
            warning("DaxBlockContainer::loadFromFiles: Failed to open file: %s",
                    filename.toString(Common::Path::kNativeSeparator).c_str());
            delete file;
        }
    }
}

DaxBlock *DaxBlockContainer::getBlockById(uint8 blockId) {
    if (_blocks.contains(blockId)) {
        return _blocks[blockId].get();
    }
    return nullptr;
}

bool DaxBlockContainer::hasBlock(uint8 blockId) const {
    return _blocks.contains(blockId);
}

uint32 DaxBlockContainer::getBlockCount() const {
    return _blocks.size();
}

Common::Array<uint8> DaxBlockContainer::getBlockIds() const {
    Common::Array<uint8> ids;
    for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
        ids.push_back(it->_key);
    }
    return ids;
}

void DaxBlockContainer::clear() {
    _blocks.clear();
}

void DaxBlockContainer::setContentType(ContentType contentType) {
    _contentType = contentType;
}

ContentType DaxBlockContainer::getContentType() const {
    return _contentType;
}

} // namespace Data
} // namespace Goldbox
