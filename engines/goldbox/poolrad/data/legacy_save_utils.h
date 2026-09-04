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

// Formats a character name into an 8.3-safe legacy base filename
// (spaces stripped, truncated to 8 characters).
Common::String formatLegacyBaseFilename(const Common::String &name);

// Appends a line to a text file under the legacy save directory,
// creating the directory and/or file as needed. Returns false on I/O failure.
bool appendLegacyTextFileLine(const Common::Path &savePath,
		const Common::String &fileName, const Common::String &line);

// Persists a newly created character to its legacy .CHA/.ITM/.SPC files
// and appends its name to CHARLIST.TXT under the legacy save directory.
// Creation-time helper only; PoolradCharacter itself stays unaware of
// where/how it gets written to disk.
bool saveNewCharacter(PoolradCharacter *pc, const Common::Path &savePath);

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_DATA_LEGACY_SAVE_UTILS_H
