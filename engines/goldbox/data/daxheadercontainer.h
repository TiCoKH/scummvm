#ifndef GOLD_BOX_DAXHEADERCONTAINER_H
#define GOLD_BOX_DAXHEADERCONTAINER_H

#include "common/array.h"
#include "common/stream.h"
#include "daxblock.h"

namespace GoldBox {

    class DaxHeaderContainer {
    private:
        Common::Array<DaxHeader> headers;
        uint16 dataOffset;
        static constexpr int headerEntrySize = 9;

    public:
        void parseHeaders(Common::ReadStream &stream);
        const Common::Array<DaxHeader>& getHeaders() const;
        uint16 getFileDataOffset() const;
    };

} // namespace GoldBox

#endif // GOLD_BOX_DAXHEADERCONTAINER_H
