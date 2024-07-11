#include "daxheadercontainer.h"

namespace GoldBox {

    void DaxHeaderContainer::parseHeaders(Common::ReadStream &stream) {
        uint16 headersBlockSize = stream.readUint16LE();
        dataOffset = headersBlockSize + 2;

        int numberOfHeaders = headersBlockSize / headerEntrySize;
        headers.clear();
        headers.resize(numberOfHeaders);

        for (int i = 0; i < numberOfHeaders; ++i) {
            DaxHeader header;
            header.id = stream.readByte();
            header.offset = stream.readUint32LE();
            header.rawSize = stream.readUint16LE();
            header.compSize = stream.readUint16LE();
            headers[i] = header;
        }
    }

    const Common::Array<DaxHeader>& DaxHeaderContainer::getHeaders() const {
        return headers;
    }

    uint16 DaxHeaderContainer::getFileDataOffset() const {
        return dataOffset;
    }

} // namespace GoldBox
