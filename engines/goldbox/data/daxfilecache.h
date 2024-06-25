// DaxFileCache.h
#ifndef GOLD_BOX_DAXFILECACHE_H
#define GOLD_BOX_DAXFILECACHE_H

#include "common/array.h"
#include "common/stream.h"
#include "common/file.h"
#include "common/hashmap.h"
#include "daxblock.h"

namespace GoldBox {

    class DaxFileCache {
    private:
        Common::HashMap<int, Common::Array<uint8>> entries;
        Common::String filename;

        void loadFile();
        void decode(int decodeSize, int dataLength, uint8 *output_ptr, const uint8 *input_ptr);

    public:
        DaxFileCache(const Common::String &baseFilename);
        Common::Array<uint8> getData(int block_id);
    };

} // namespace GoldBox

#endif // GOLD_BOX_DAXFILECACHE_H
