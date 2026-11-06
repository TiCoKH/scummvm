/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GOLDBOX_POOLRAD_DATA_POOLRAD_LEGACY_ROUNDTRIP_H
#define GOLDBOX_POOLRAD_DATA_POOLRAD_LEGACY_ROUNDTRIP_H

#include "common/array.h"
#include "goldbox/poolrad/data/poolrad_legacy_layout.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

/**
 * Round-trip byte-diff classifier for the DOS legacy formats (GB13-034 C).
 *
 * Classifies each differing byte between an original DOS record and the record
 * ScummVM re-serialized after loading it:
 *   PRESERVED  - identical (not a diff; never reported)
 *   NORMALIZED - inside a pointer-placeholder / FAVAIL / unknown-pad span that
 *                the codec canonicalizes (acceptable)
 *   ERRONEOUS  - any other differing byte: a data field changed (FAILURE)
 *
 * The classifier is pure (no I/O, no globals) so it is fully headless-testable.
 * REBUILT fields (derived values recomputed on load) that happen to change are
 * caller-specific; the canonical per-character round-trip in GB13-034 loads and
 * immediately re-saves without recalculation, so no data field should be
 * rebuilt - any such change is therefore surfaced as ERRONEOUS and recorded as
 * an OPEN issue rather than silently accepted.
 */
class LegacyRoundTrip {
public:
	struct Span {
		uint32 offset;
		uint32 length;
		bool contains(uint32 pos) const {
			return pos >= offset && pos < offset + length;
		}
	};

	struct Diff {
		uint32 offset;
		uint8 original;
		uint8 rewritten;
	};

	struct Result {
		uint32 comparedBytes;
		uint32 normalizedCount;
		Common::Array<Diff> erroneous; ///< data-field diffs = FAILURES

		Result() : comparedBytes(0), normalizedCount(0) {}
		bool ok() const { return erroneous.empty(); }
	};

	/// Build the normalization mask (pointer-placeholder / FAVAIL / pad spans)
	/// for the 285-byte character record.
	static Common::Array<Span> characterNormalizedSpans() {
		Common::Array<Span> spans;
		spans.push_back(makeSpan(LegacyLayout::kCharEffectsAddress,
				LegacyLayout::kCharEffectsAddrLen));
		spans.push_back(makeSpan(0x086, LegacyLayout::kCharUnknown086Len));
		spans.push_back(makeSpan(LegacyLayout::kCharItemsAddress,
				LegacyLayout::kCharItemsAddrLen));
		spans.push_back(makeSpan(LegacyLayout::kCharEquippedOffsets,
				LegacyLayout::kCharEquippedLen));
		spans.push_back(makeSpan(LegacyLayout::kCharNextCharAddr,
				LegacyLayout::kCharNextCombatLen));
		return spans;
	}

	/// Build the normalization mask for one 9-byte effect node: the trailing
	/// far-pointer (bytes 5..8) is a heap address the codec zeroes.
	static Common::Array<Span> effectNodeNormalizedSpans() {
		Common::Array<Span> spans;
		spans.push_back(makeSpan(LegacyLayout::kEffectNextPtr,
				LegacyLayout::kEffectNextPtrLen));
		return spans;
	}

	/// Effect file (.SPC) mask: repeat the per-node pointer span for each node.
	static Common::Array<Span> effectFileNormalizedSpans(uint32 fileSize) {
		Common::Array<Span> spans;
		const uint32 nodes = fileSize / LegacyLayout::kEffectNodeSize;
		for (uint32 i = 0; i < nodes; ++i) {
			const uint32 base = i * LegacyLayout::kEffectNodeSize;
			spans.push_back(makeSpan(base + LegacyLayout::kEffectNextPtr,
					LegacyLayout::kEffectNextPtrLen));
		}
		return spans;
	}

	/// The .ITM item record has one pointer placeholder per record (the legacy
	/// next-address used as the equip key); repeat it per record.
	static Common::Array<Span> itemFileNormalizedSpans(uint32 fileSize) {
		Common::Array<Span> spans;
		const uint32 recs = fileSize / LegacyLayout::kItemRecordSize;
		for (uint32 i = 0; i < recs; ++i) {
			const uint32 base = i * LegacyLayout::kItemRecordSize;
			spans.push_back(makeSpan(base + LegacyLayout::kItemNextAddress, 4));
		}
		return spans;
	}

	/// Compare two equally sized buffers, classifying each differing byte.
	/// Diffs inside a normalized span are counted as NORMALIZED; everything
	/// else is ERRONEOUS. Length mismatch is itself an ERRONEOUS failure
	/// reported as a synthetic diff at the first out-of-range offset.
	static Result compare(const byte *original, uint32 originalLen,
			const byte *rewritten, uint32 rewrittenLen,
			const Common::Array<Span> &normalized) {
		Result result;

		if (originalLen != rewrittenLen) {
			Diff d;
			d.offset = MIN(originalLen, rewrittenLen);
			d.original = 0;
			d.rewritten = 0;
			result.erroneous.push_back(d);
			return result;
		}

		result.comparedBytes = originalLen;
		for (uint32 i = 0; i < originalLen; ++i) {
			if (original[i] == rewritten[i])
				continue;
			if (isNormalized(i, normalized)) {
				++result.normalizedCount;
				continue;
			}
			Diff d;
			d.offset = i;
			d.original = original[i];
			d.rewritten = rewritten[i];
			result.erroneous.push_back(d);
		}
		return result;
	}

private:
	static Span makeSpan(uint32 offset, uint32 length) {
		Span s;
		s.offset = offset;
		s.length = length;
		return s;
	}

	static bool isNormalized(uint32 pos, const Common::Array<Span> &spans) {
		for (uint i = 0; i < spans.size(); ++i) {
			if (spans[i].contains(pos))
				return true;
		}
		return false;
	}
};

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_DATA_POOLRAD_LEGACY_ROUNDTRIP_H
