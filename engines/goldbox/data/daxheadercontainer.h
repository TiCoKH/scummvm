#ifndef GOLDBOX_DATA_DAXHEADERCONTAINER_H
#define GOLDBOX_DATA_DAXHEADERCONTAINER_H

#include "common/array.h"
#include "common/stream.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {
namespace Data {

    class DaxHeaderContainer {
    private:
        Common::Array<DaxHeader> headers;
        uint16 dataOffset;

    public:
        void parseHeadersLE(Common::ReadStream &stream);
        void parseHeadersBE(Common::ReadStream &stream);
        const Common::Array<DaxHeader>& getHeaders() const;
        uint16 getFileDataOffset() const;
    };

} // namespace Data
} // namespace GoldBox

#endif // GOLDBOX_DAXHEADERCONTAINER_H
