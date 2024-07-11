#include "common/hash-str.h"
#include "common/util.h"
#include "common/str.h"
#include "daxcache.h"
#include "daxheadercontainer.h"

namespace GoldBox {

    DaxCache g_daxCache;

    static Common::HashMap<ContentType, Common::Array<DaxBlock*>> contentCache;

    DaxCache::DaxCache() {
        // Initialize the cache
    }

    DaxCache::~DaxCache() {
        clearCache();
    }

    ContentType DaxCache::determineContentType(const Common::String &filename) {
        if (filename.contains("8x8d")) return ContentType::_8X8D;
        if (filename.contains("back")) return ContentType::BACK;
        if (filename.contains("bigpic")) return ContentType::BIGPIC;
        if (filename.contains("cbody")) return ContentType::CBODY;
        if (filename.contains("body")) return ContentType::BODY;
        if (filename.contains("comspr")) return ContentType::COMSPR;
        if (filename.contains("ecl")) return ContentType::ECL;
        if (filename.contains("geo")) return ContentType::GEO;
        if (filename.contains("chead")) return ContentType::CHEAD;
        if (filename.contains("head")) return ContentType::HEAD;
        if (filename.contains("moncha")) return ContentType::MONCHA;
        if (filename.contains("pic")) return ContentType::PIC;
        if (filename.contains("sprit")) return ContentType::SPRIT;
        if (filename.contains("title")) return ContentType::TITLE;
        if (filename.contains("walldef")) return ContentType::WALLDEF;
        return ContentType::UNKNOWN;
    }

    void DaxCache::loadFile(const Common::String &filename) {
        Common::File file;
        if (!file.open(filename + ".dax")) {
            return;
        }

        GoldBox::DaxHeaderContainer dhc;
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
                daxBlock->data = dax_data;
            } else {
                Common::Array<uint8> decode_data(dhe.rawSize);
                decodeRLE(dhe.compSize, decode_data.data(), dax_data.data());
                daxBlock->data = decode_data;
            }
            daxBlock->blockId = dhe.id;
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
        if (contentCache.contains(type)) {
            for (auto &block : contentCache[type]) {
                if (block->blockId == block_id) {
                    return true;
                }
            }
        }
        return false;
    }

    Common::Array<uint8> DaxCache::getData(ContentType type, int block_id) {
        if (contentCache.contains(type)) {
            for (auto &block : contentCache[type]) {
                if (block->blockId == block_id) {
                    return block->data;
                }
            }
        }
        return Common::Array<uint8>();
    }

    void DaxCache::clearCache() {
        for (auto &entry : contentCache) {
            for (auto &block : entry._value) {
                delete block;
            }
            entry._value.clear();
        }
        contentCache.clear();
    }

} // namespace GoldBox
