/* ScummVM - Graphic Adventure Engine
 *
 * Legacy Poolrad save helper utilities.
 */

#include "common/fs.h"

#include "goldbox/poolrad/data/legacy_save_utils.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

namespace {

static char toUpperAscii(char c) {
	if (c >= 'a' && c <= 'z')
		return static_cast<char>(c - ('a' - 'A'));
	return c;
}

static bool containsBaseName(const Common::Array<Common::String> &usedBases,
		const Common::String &candidate) {
	for (uint i = 0; i < usedBases.size(); ++i) {
		if (usedBases[i] == candidate)
			return true;
	}
	return false;
}

static bool hasPrefixNoCase(const Common::String &s,
		const Common::String &prefix) {
	if (s.size() < prefix.size())
		return false;

	for (uint i = 0; i < prefix.size(); ++i) {
		if (toUpperAscii(s[i]) != toUpperAscii(prefix[i]))
			return false;
	}

	return true;
}

} // namespace

Common::String makeLegacyCharacterBaseName(
		const PoolradCharacter *pc, char slotLetter, uint8 ordinal,
		const Common::Array<Common::String> &usedBases) {
	(void)pc;

	// Match original legacy companion naming convention used by x86 saves:
	// CHRDAT<slot><n> (example: SAVGAMF.DAT -> CHRDATF1..CHRDATF8).
	Common::String base = Common::String::format("CHRDAT%c%u",
		slotLetter, (unsigned)ordinal);
	if (!containsBaseName(usedBases, base))
		return base;

	for (uint suffix = 2; suffix < 1000; ++suffix) {
		Common::String candidate = base;
		const Common::String suffixStr = Common::String::format("%u",
			(unsigned)suffix);
		while (candidate.size() + suffixStr.size() > 0x28)
			candidate.deleteLastChar();
		candidate += suffixStr;
		if (!containsBaseName(usedBases, candidate))
			return candidate;
	}

	return base;
}

bool openLegacyCompanionStream(const Common::Path &savePath,
		const Common::String &base, char slotLetter,
		const Common::String &extUpper, const Common::String &extLower,
		Common::Path &resolvedPath, Common::SeekableReadStream *&stream) {
	stream = nullptr;
	resolvedPath = Common::Path();

	Common::Array<Common::String> candidateBases;
	candidateBases.push_back(base);

	if (hasPrefixNoCase(base, "CHRDATA") && base.size() > 7) {
		const Common::String suffix = base.substr(7);
		candidateBases.push_back(Common::String::format("CHRDAT%c%s",
			slotLetter, suffix.c_str()));
	} else if (hasPrefixNoCase(base, "CHRDAT") && base.size() > 7) {
		const Common::String suffix = base.substr(7);
		candidateBases.push_back(Common::String::format("CHRDATA%s",
			suffix.c_str()));
	}

	for (uint i = 0; i < candidateBases.size(); ++i) {
		const Common::String &candidateBase = candidateBases[i];
		const Common::Path pUpper = savePath / (candidateBase + extUpper);
		Common::FSNode nUpper(pUpper);
		if (nUpper.exists() && !nUpper.isDirectory()) {
			stream = nUpper.createReadStream();
			if (stream) {
				resolvedPath = pUpper;
				return true;
			}
		}

		const Common::Path pLower = savePath / (candidateBase + extLower);
		Common::FSNode nLower(pLower);
		if (nLower.exists() && !nLower.isDirectory()) {
			stream = nLower.createReadStream();
			if (stream) {
				resolvedPath = pLower;
				return true;
			}
		}
	}

	return false;
}

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox
