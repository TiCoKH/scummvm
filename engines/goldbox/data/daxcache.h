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

        DaxCache();
        ~DaxCache();
        DaxCache(const DaxCache &) = delete;
        DaxCache &operator=(const DaxCache &) = delete;
        ContentType determineContentType(const Common::Path &filename);
        void decodeRLE(int dataLength, uint8 *output_ptr, const uint8 *input_ptr);
        bool blockExists(ContentType type, int block_id);

    public:
        static DaxCache &getInstance();

        void loadFile(const Common::Path &filename);
        DaxBlock* getBlock(ContentType type, int block_id);
        static void clearCache();
    };

} // namespace GoldBox

#endif // GOLDBOX_DAXCACHE_H
