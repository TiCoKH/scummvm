#ifndef GOLD_BOX_DAXCACHE_H
#define GOLD_BOX_DAXCACHE_H

#include "common/hashmap.h"
#include "common/str.h"
#include "daxfilecache.h"

namespace GoldBox {

    class DaxCache {
    private:
        static Common::HashMap<Common::String, DaxFileCache *> fileCache;

    public:
        static Common::Array<uint8> loadDax(const Common::String &baseFilename, int block_id);
    };

} // namespace GoldBox

#endif // GOLD_BOX_DAXCACHE_H
