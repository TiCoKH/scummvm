#include "daxcache.h"

namespace GoldBox {

    Common::HashMap<Common::String, DaxFileCache *> DaxCache::fileCache;

    Common::Array<uint8> DaxCache::loadDax(const Common::String &baseFilename, int block_id) {
        DaxFileCache *dfc;

        Common::String lowerBaseFilename = baseFilename;
        lowerBaseFilename.toLowercase();

        if (!fileCache.contains(lowerBaseFilename)) {
            dfc = new DaxFileCache(lowerBaseFilename);
            fileCache.setVal(lowerBaseFilename, dfc);
        } else {
            dfc = fileCache[lowerBaseFilename];
        }

        return dfc->getData(block_id);
    }

} // namespace GoldBox
