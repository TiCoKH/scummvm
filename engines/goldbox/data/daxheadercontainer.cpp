#include "goldbox/data/daxheadercontainer.h"

namespace Goldbox {
namespace Data {

    void DaxHeaderContainer::parseHeadersLE(Common::ReadStream &stream) {
        uint16 headersBlockSize = stream.readUint16LE();
        dataOffset = headersBlockSize + 2;

        int numberOfHeaders = headersBlockSize / 9;
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

    void DaxHeaderContainer::parseHeadersBE(Common::ReadStream &stream) {
        uint16 headersBlockSize = stream.readUint16BE();
        dataOffset = headersBlockSize + 2;

        int numberOfHeaders = headersBlockSize / 10;
        headers.clear();
        headers.resize(numberOfHeaders);

        for (int i = 0; i < numberOfHeaders; ++i) {
            DaxHeader header;
            header.id = stream.readUint16BE();
            header.offset = stream.readUint32BE();
            header.rawSize = stream.readUint16BE();
            header.compSize = stream.readUint16BE();
            headers[i] = header;
        }
    }

    const Common::Array<DaxHeader>& DaxHeaderContainer::getHeaders() const {
        return headers;
    }

    uint16 DaxHeaderContainer::getFileDataOffset() const {
        return dataOffset;
    }

} // namespace Data
} // namespace GoldBox