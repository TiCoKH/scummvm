/* ScummVM - Graphic Adventure Engine
 *
 * Legacy Poolrad save helper utilities.
 */

#ifndef GOLDBOX_POOLRAD_DATA_LEGACY_SAVE_UTILS_H
#define GOLDBOX_POOLRAD_DATA_LEGACY_SAVE_UTILS_H

#include "common/array.h"
#include "common/path.h"
#include "common/stream.h"
#include "common/str.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

class PoolradCharacter;

Common::String makeLegacyCharacterBaseName(
		const PoolradCharacter *pc, char slotLetter, uint8 ordinal,
		const Common::Array<Common::String> &usedBases);

bool openLegacyCompanionStream(const Common::Path &savePath,
		const Common::String &base, char slotLetter,
		const Common::String &extUpper, const Common::String &extLower,
		Common::Path &resolvedPath, Common::SeekableReadStream *&stream);

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_DATA_LEGACY_SAVE_UTILS_H
