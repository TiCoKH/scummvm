#include "common/hash-str.h"
#include "common/util.h"
#include "common/str.h"
#include "common/file.h"
#include "goldbox/core/file.h"
#include "goldbox/data/daxcache.h"
#include "goldbox/data/daxheadercontainer.h"

namespace Goldbox {

    Common::HashMap<CacheKey, DaxBlock*, CacheKeyHash> DaxCache::contentCache;

    DaxCache::DaxCache() {
        // Initialize the cache
    }

    DaxCache::~DaxCache() {
        clearCache();
    }

    ContentType DaxCache::determineContentType(const Common::Path &filename) {
        Common::String s = filename.toString();
        if (s.contains("8x8d")) return ContentType::_8X8D;
        if (s.contains("back")) return ContentType::BACK;
        if (s.contains("bigpic")) return ContentType::BIGPIC;
        if (s.contains("cbody")) return ContentType::CBODY;
        if (s.contains("body")) return ContentType::BODY;
        if (s.contains("comspr")) return ContentType::COMSPR;
        if (s.contains("ecl")) return ContentType::ECL;
        if (s.contains("geo")) return ContentType::GEO;
        if (s.contains("chead")) return ContentType::CHEAD;
        if (s.contains("head")) return ContentType::HEAD;
        if (s.contains("moncha")) return ContentType::MONCHA;
        if (s.contains("pic")) return ContentType::PIC;
        if (s.contains("sprit")) return ContentType::SPRIT;
        if (s.contains("title")) return ContentType::TITLE;
        if (s.contains("walldef")) return ContentType::WALLDEF;
        return ContentType::UNKNOWN;
    }

    void DaxCache::loadFile(const Common::Path &filename) {
        Goldbox::File file;
        if (!file.open(filename)) {
            return;
        }

        Goldbox::DaxHeaderContainer dhc;
        dhc.parseHeaders(file);
        uint16 data_off = dhc.getFileDataOffset();
        ContentType contentType = determineContentType(filename);

        for (const auto &dhe : dhc.getHeaders()) {
             if (blockExists(contentType, dhe.id)) {
                continue;
            }
            file.seek(data_off + dhe.offset, SEEK_SET);
            Common::Array<uint8> dax_data(dhe.compSize);
            file.read(dax_data.data(), dhe.compSize);

            // Create the appropriate DaxBlock based on the content type
            DaxBlock* daxBlock = DaxBlock::createDaxBlock(contentType);
            if (!daxBlock) {
                // Handle unknown content type
                continue;
            }

            if (dhe.rawSize <= 0) {
                daxBlock->_data = dax_data;
            } else {
                Common::Array<uint8> decode_data(dhe.rawSize);
                decodeRLE(dhe.compSize, decode_data.data(), dax_data.data());
                daxBlock->_data = decode_data;
            }
            daxBlock->blockId = dhe.id;
            daxBlock->adjust();
            contentCache[CacheKey(contentType, dhe.id)] = daxBlock;
        }
        file.close();
    }

   void DaxCache::decodeRLE(int dataLength, uint8 *output_ptr, const uint8 *input_ptr) {
        int input_index = 0;
        int output_index = 0;

        while (input_index < dataLength) {
            int8 run_length = static_cast<int8>(input_ptr[input_index]);

            if (run_length >= 0) {
                memcpy(output_ptr + output_index, input_ptr + input_index + 1, run_length + 1);
                input_index += run_length + 2;
                output_index += run_length + 1;
            } else {
                run_length = -run_length;
                memset(output_ptr + output_index, input_ptr[input_index + 1], run_length);
                input_index += 2;
                output_index += run_length;
            }
        }
    }

    bool DaxCache::blockExists(ContentType type, int block_id) {
        return contentCache.contains(CacheKey(type, block_id));
    }

    DaxBlock* DaxCache::getBlock(ContentType type, int block_id) {
    CacheKey key = CacheKey(type, block_id);
    if (contentCache.contains(key)) {
        return contentCache[key];
    }
    return nullptr;
}

    void DaxCache::clearCache() {
        for (auto &entry : contentCache) {
            delete entry._value;
        }
        contentCache.clear();
    }

} // namespace Goldbox
