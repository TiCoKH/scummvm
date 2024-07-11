#ifndef GOLD_BOX_DAXCACHE_H
#define GOLD_BOX_DAXCACHE_H

#include "common/hashmap.h"
#include "common/str.h"
#include "daxblock.h"
#include <memory>

namespace GoldBox {

    typedef std::pair<ContentType, int> CacheKey;

    class DaxCache {
    private:
        static Common::HashMap<CacheKey, DaxBlock*> contentCache;

        ContentType determineContentType(const Common::String &filename);
        void loadFile(const Common::String &filename);
        void decodeRLE(int dataLength, uint8 *output_ptr, const uint8 *input_ptr);
        bool blockExists(ContentType type, int block_id);

    public:
        DaxCache();
        ~DaxCache();
        Common::Array<uint8> getData(ContentType type, int block_id);
        static void clearCache();
    };

    extern DaxCache g_daxCache;

} // namespace GoldBox

#endif // GOLD_BOX_DAXCACHE_H
