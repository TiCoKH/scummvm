// DaxFileCache.cpp
#include "daxfilecache.h"
#include "common/hash-str.h"
#include "common/util.h"
#include "common/str.h"

namespace GoldBox {

    DaxFileCache::DaxFileCache(const Common::String &baseFilename) : filename(baseFilename + ".dax") {
        loadFile();
    }

    void DaxFileCache::loadFile() {
        uint16 dataOffset = 0;
        uint16 headersBlockSize = 0;

        Common::File file;
        if (!file.open(filename)) {
            return;
        }

        headersBlockSize = file.readUint16LE();
        dataOffset = headersBlockSize + 2;  // Assuming the offset is right after the headers block

        Common::Array<GoldBox::DaxHeader> headers;
        const int headerEntrySize = 9;
        uint8 numberOfEntries = headersBlockSize / headerEntrySize;
        headers.resize(numberOfEntries);

        for (uint8 i = 0; i < numberOfEntries; ++i) {
            GoldBox::DaxHeader dhe;
            dhe.id = file.readByte();
            dhe.offset = file.readUint32LE();
            dhe.rawSize = file.readUint16LE();
            dhe.compSize = file.readUint16LE();
            headers[i] = dhe;
        }

        for (const auto &dhe : headers) {
            Common::Array<uint8> comp(dhe.compSize);
            Common::Array<uint8> raw(dhe.rawSize);

            file.seek(dataOffset + dhe.offset, SEEK_SET);
            file.read(comp.begin(), dhe.compSize);

            decode(dhe.rawSize, dhe.compSize, raw.begin(), comp.begin());

            entries.setVal(dhe.id, raw);
        }

        file.close();
    }

    void DaxFileCache::decode(int decodeSize, int dataLength, uint8 *output_ptr, const uint8 *input_ptr) {
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

    Common::Array<uint8> DaxFileCache::getData(int block_id) {
        if (entries.contains(block_id)) {
            return entries[block_id];
        }
        return Common::Array<uint8>();
    }

} // namespace GoldBox
