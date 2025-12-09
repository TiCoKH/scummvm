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

#ifndef GOLDBOX_DATA_DAXBLOCKCONTAINER_H
#define GOLDBOX_DATA_DAXBLOCKCONTAINER_H

#include "common/array.h"
#include "common/hashmap.h"
#include "common/path.h"
#include "common/ptr.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {

// Forward declaration
class File;

namespace Data {

/**
 * Container for managing DAX blocks from multiple files.
 * Ensures each block ID is present only once. When loading from
 * multiple files, blocks are kept from the first file they appear in.
 */
class DaxBlockContainer {
private:
    Common::HashMap<uint8, Common::SharedPtr<DaxBlock>> _blocks;
    ContentType _contentType;

public:
    DaxBlockContainer();
    explicit DaxBlockContainer(ContentType contentType);
    ~DaxBlockContainer();

    /**
     * Load DAX blocks from a file.
     * Blocks with IDs that already exist in the container will be skipped.
     * @param file The DAX file to load blocks from
     */
    void loadFromFile(File *file);

    /**
     * Load DAX blocks from multiple files.
     * Files are processed in order. Blocks are kept from the first file
     * they appear in; duplicate block IDs in later files are skipped.
     * @param filenames Array of file paths to load
     */
    void loadFromFiles(const Common::Array<Common::Path> &filenames);

    /**
     * Get a block by its ID.
     * @param blockId The ID of the block to retrieve
     * @return Pointer to the block, or nullptr if not found
     */
    DaxBlock *getBlockById(uint8 blockId);

    /**
     * Check if a block with the given ID exists.
     * @param blockId The ID to check
     * @return true if the block exists, false otherwise
     */
    bool hasBlock(uint8 blockId) const;

    /**
     * Get the number of blocks in the container.
     * @return The number of unique blocks
     */
    uint32 getBlockCount() const;

    /**
     * Get all block IDs in the container.
     * @return Array of all block IDs
     */
    Common::Array<uint8> getBlockIds() const;

    /**
     * Clear all blocks from the container.
     */
    void clear();

    /**
     * Set the content type for blocks in this container.
     * @param contentType The type of content this container manages
     */
    void setContentType(ContentType contentType);

    /**
     * Get the content type for this container.
     * @return The content type
     */
    ContentType getContentType() const;
};

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_DAXBLOCKCONTAINER_H
