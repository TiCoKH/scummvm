#ifndef GOLDBOX_DAXCACHE_H
#define GOLDBOX_DAXCACHE_H

#include "common/path.h"
#include "common/hashmap.h"
#include "common/str.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {

    struct CacheKey {
        ContentType type;
        int id;

        CacheKey(ContentType t, int i) : type(t), id(i) {}

        bool operator==(const CacheKey& other) const {
            return type == other.type && id == other.id;
        }
    };

    struct CacheKeyHash {
        size_t operator()(const CacheKey &key) const {
            // Custom hash function
            size_t hash = 17;
            hash = hash * 31 + static_cast<int>(key.type);
            hash = hash * 31 + key.id;
            return hash;
        }
    };

    class DaxCache {
    private:
        static Common::HashMap<CacheKey, DaxBlock*, CacheKeyHash> contentCache;

        ContentType determineContentType(const Common::Path &filename);
        void decodeRLE(int dataLength, uint8 *output_ptr, const uint8 *input_ptr);
        bool blockExists(ContentType type, int block_id);

    public:
        DaxCache();
        ~DaxCache();

        void loadFile(const Common::Path &filename);
        DaxBlock* getBlock(ContentType type, int block_id);
        static void clearCache();
    };

extern DaxCache g_daxCache;

} // namespace GoldBox

#endif // GOLDBOX_DAXCACHE_H
