#ifndef GOLDBOX_DAXCACHE_H
#define GOLDBOX_DAXCACHE_H

#include "common/hashmap.h"
#include "common/str.h"
#include "goldbox/data/daxblock.h"
#include <memory>

namespace Goldbox {

    typedef std::pair<ContentType, int> CacheKey;

    class DaxCache {
    private:
        static Common::HashMap<CacheKey, DaxBlock*> contentCache;

        ContentType determineContentType(const Common::Path &filename);
        void decodeRLE(int dataLength, uint8 *output_ptr, const uint8 *input_ptr);
        bool blockExists(ContentType type, int block_id);

    public:
        DaxCache();
        ~DaxCache();
        void loadFile(const Common::Path &filename);
        Common::Array<uint8> getData(ContentType type, int block_id);
        static void clearCache();
    };

    extern DaxCache g_daxCache;

} // namespace GoldBox

#endif // GOLDBOX_DAXCACHE_H
