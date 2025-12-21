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

#include "goldbox/gfx/icon.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/vm_interface.h"
#include "common/debug.h"

namespace Goldbox {
namespace Gfx {

// Helper: ensure we don't double-apply size offsets when stored sprite ids
// already include the large-size offset. If size marks large and the id is
// still in the small range, add the large offset; otherwise leave as-is.
static uint8 resolveSpriteId(uint8 storedId, uint8 size) {
	if (size == Icon::SIZE_LARGE && storedId < Icon::SIZE_OFFSET_LARGE)
		return static_cast<uint8>(storedId + Icon::SIZE_OFFSET_LARGE);
	return storedId;
}

// Icon member functions
Icon::Icon(const Data::CombatIconData &iconData,
                             IconState state)
    : _size(static_cast<IconSize>(iconData.iconSize)),
      _state(state),
      _direction(ICON_DIRECTION_RIGHT),
	  _renderer(nullptr),
      _composite(nullptr),
      _readyIcon(nullptr),
      _actionIcon(nullptr),
      _readyIconFlipped(nullptr),
      _actionIconFlipped(nullptr),
      _readyHeadPic(nullptr),
      _readyBodyPic(nullptr),
      _actionHeadPic(nullptr),
      _actionBodyPic(nullptr) {
				debug("Icon::Icon(CombatIconData): iconSize=%u head(base)=%u body(base)=%u state=%s",
				      (unsigned)iconData.iconSize,
				      (unsigned)iconData.iconHead,
				      (unsigned)iconData.iconBody,
				      (state == ICON_STATE_ATTACK ? "ATTACK" : "READY"));

	uint8 readyHeadId = resolveSpriteId(iconData.iconHead, _size);
	uint8 readyBodyId = resolveSpriteId(iconData.iconBody, _size);
	uint8 actionHeadId = static_cast<uint8>(readyHeadId + ACTION_OFFSET);
	uint8 actionBodyId = static_cast<uint8>(readyBodyId + ACTION_OFFSET);
				debug("  - Resolved IDs: readyHead=%u readyBody=%u actionHead=%u actionBody=%u (size=%u)",
				      (unsigned)readyHeadId, (unsigned)readyBodyId,
				      (unsigned)actionHeadId, (unsigned)actionBodyId,
				      (unsigned)_size);

	// Load base sprites from DAX containers (cached)
	if (!loadBaseSprites(readyHeadId, readyBodyId, actionHeadId, actionBodyId)) {
		// Fallback: create empty placeholder if sprites missing
		_readyIcon = new Pic(24, 24);
		_readyIcon->clear(0);
		_actionIcon = new Pic(24, 24);
		_actionIcon->clear(0);
	} else {
		// Build composite icons using cached base sprites
		// Ready state should use isAction=false; Action state uses isAction=true
		_readyIcon = buildComposite(false, iconData);
		_actionIcon = buildComposite(true, iconData);
	}

	// Build vertically flipped versions
	_readyIconFlipped = createFlipped(_readyIcon);
	_actionIconFlipped = createFlipped(_actionIcon);

	// Set current composite based on action state and orientation
	updateComposite();

	// Debug: Dump hex content of composite icons
	debugDumpPicHex(_readyIcon, "ReadyIcon (CombatIconData constructor)");
	debugDumpPicHex(_actionIcon, "ActionIcon (CombatIconData constructor)");
}

Icon::Icon(const Pic *readyPic, const Pic *actionPic)
	    : _size(ICON_SIZE_SMALL),
	      _state(ICON_STATE_READY),
	      _direction(ICON_DIRECTION_RIGHT),
	      _renderer(nullptr),
	      _composite(nullptr),
	      _readyIcon(nullptr),
	      _actionIcon(nullptr),
	      _readyIconFlipped(nullptr),
	      _actionIconFlipped(nullptr),
	      _readyHeadPic(nullptr),
	      _readyBodyPic(nullptr),
	      _actionHeadPic(nullptr),
	      _actionBodyPic(nullptr) {

	debug("Icon::Icon(Pic, Pic) Constructor - Loading from provided Pic pointers");
	debug("  readyPic=%p actionPic=%p", readyPic, actionPic);
	debug("Icon::Icon(Pic, Pic) Constructor - Loading from provided Pic pointers");
	debug("  readyPic=%p actionPic=%p", readyPic, actionPic);

	// Use provided pics as sources; fall back to an empty placeholder if both are missing
	const Pic *readySource = readyPic ? readyPic : actionPic;
	Pic *placeholder = nullptr;
	if (!readySource) {
		debug("  - Both readyPic and actionPic are NULL, creating placeholder");
		placeholder = new Pic(24, 24);
		placeholder->clear(0);
		readySource = placeholder;
	} else {
		debug("  - Using readyPic (w=%d, h=%d)", readySource->w, readySource->h);
	}

	const Pic *actionSource = actionPic ? actionPic : readySource;
	if (actionSource != readySource) {
		debug("  - Using actionPic (w=%d, h=%d)", actionSource->w, actionSource->h);
	} else {
		debug("  - Reusing readyPic as actionPic");
	}

	// Size categorization should not rely on hardcoded pixel constants.
	// Use DAX metadata to classify SMALL vs LARGE by comparing typical body sprite sizes.
	debug("  - Provided art dimensions: %dx%d", readySource->w, readySource->h);
	{
		Data::DaxBlockPic *smallBody = static_cast<Data::DaxBlockPic *>(
			VmInterface::getDaxCBody().getBlockById(0));
		Data::DaxBlockPic *largeBody = static_cast<Data::DaxBlockPic *>(
			VmInterface::getDaxCBody().getBlockById(0 + SIZE_OFFSET_LARGE));
		if (smallBody && largeBody) {
			if (readySource->w == (int)largeBody->width && readySource->h == (int)largeBody->height) {
				_size = ICON_SIZE_LARGE;
				debug("  - Classified as LARGE via DAX body dims (%dx%d)", (int)largeBody->width, (int)largeBody->height);
			} else {
				_size = ICON_SIZE_SMALL;
				debug("  - Classified as SMALL via DAX body dims (%dx%d)", (int)smallBody->width, (int)smallBody->height);
			}
		} else {
			// If DAX metadata is not available, default to SMALL; pixel dims remain authoritative for rendering.
			_size = ICON_SIZE_SMALL;
			debug("  - DAX metadata unavailable; defaulting to SMALL category");
		}
	}

	_readyIcon = readySource->clone();
	debug("  - Cloned readyIcon: %p", _readyIcon);

	_actionIcon = actionSource->clone();
	debug("  - Cloned actionIcon: %p", _actionIcon);

	_readyIconFlipped = createFlipped(_readyIcon);
	debug("  - Created readyIconFlipped: %p", _readyIconFlipped);

	_actionIconFlipped = createFlipped(_actionIcon);
	debug("  - Created actionIconFlipped: %p", _actionIconFlipped);

	updateComposite();
	debug("  - Icon construction complete: composite=%p (state=%d, dir=%d)", _composite, _state, _direction);

	// Debug: Dump hex content of composite icons
	debugDumpPicHex(_readyIcon, "ReadyIcon (Pic constructor)");
	debugDumpPicHex(_actionIcon, "ActionIcon (Pic constructor)");

	delete placeholder;
}

Icon::Icon(uint16 blockId, IconKind kind)
	    : _size(ICON_SIZE_SMALL),
	      _state(ICON_STATE_READY),
	      _direction(ICON_DIRECTION_RIGHT),
	      _renderer(nullptr),
	      _composite(nullptr),
	      _readyIcon(nullptr),
	      _actionIcon(nullptr),
	      _readyIconFlipped(nullptr),
	      _actionIconFlipped(nullptr),
	      _readyHeadPic(nullptr),
	      _readyBodyPic(nullptr),
	      _actionHeadPic(nullptr),
	      _actionBodyPic(nullptr) {

	debug("Icon::Icon(blockId, kind) Constructor - Loading from DAX block");
	debug("  blockId=%u kind=%d", blockId, kind);

	// Get DAX manager to fetch the blocks
	Data::DaxFileManager &daxMgr = VmInterface::getDaxManager();

	// Get the appropriate DAX container based on kind
	Data::DaxBlockContainer *container = nullptr;
	switch (kind) {
	case ICON_KIND_SPRITE:
		container = &daxMgr.getComSpr();
		debug("  - Container: COMSPR");
		break;
	case ICON_KIND_CPIC:
		container = &daxMgr.getCPic();
		debug("  - Container: CPIC");
		break;
	default:
		warning("Icon::Icon(blockId): Invalid kind %d, only SPRITE and CPIC supported", kind);
		_readyIcon = new Pic(24, 24);
		_readyIcon->clear(0);
		_actionIcon = new Pic(24, 24);
		_actionIcon->clear(0);
		_readyIconFlipped = createFlipped(_readyIcon);
		_actionIconFlipped = createFlipped(_actionIcon);
		updateComposite();
		return;
	}

	// Ready state uses base blockId
	uint16 readyBlockId = blockId;
	// Action state adds 128 offset (same pattern as head/body sprites)
	uint16 actionBlockId = blockId + 128;

	debug("  - Loading readyBlockId=%u actionBlockId=%u", readyBlockId, actionBlockId);

	// Get the blocks
	Data::DaxBlock *readyBlock = container->getBlockById(readyBlockId);
	Data::DaxBlock *actionBlock = container->getBlockById(actionBlockId);

	if (!readyBlock || !actionBlock) {
		warning("Icon::Icon(blockId): Block not found readyBlock=%p actionBlock=%p", readyBlock, actionBlock);
		_readyIcon = new Pic(24, 24);
		_readyIcon->clear(0);
		_actionIcon = new Pic(24, 24);
		_actionIcon->clear(0);
		_readyIconFlipped = createFlipped(_readyIcon);
		_actionIconFlipped = createFlipped(_actionIcon);
		updateComposite();
		return;
	}

	// Cast to DaxBlockPic
	Data::DaxBlockPic *readyPicBlock = dynamic_cast<Data::DaxBlockPic *>(readyBlock);
	Data::DaxBlockPic *actionPicBlock = dynamic_cast<Data::DaxBlockPic *>(actionBlock);

	if (!readyPicBlock || !actionPicBlock) {
		warning("Icon::Icon(blockId): Block is not DaxBlockPic readyPic=%p actionPic=%p", readyPicBlock, actionPicBlock);
		_readyIcon = new Pic(24, 24);
		_readyIcon->clear(0);
		_actionIcon = new Pic(24, 24);
		_actionIcon->clear(0);
		_readyIconFlipped = createFlipped(_readyIcon);
		_actionIconFlipped = createFlipped(_actionIcon);
		updateComposite();
		return;
	}

	debug("  - readyPicBlock: %dx%d actionPicBlock: %dx%d", readyPicBlock->width, readyPicBlock->height, actionPicBlock->width, actionPicBlock->height);

	// Create Pics from blocks with appropriate remapping for CTILE
	const bool isCTile = (kind == ICON_KIND_CPIC || container->getContentType() == Data::ContentType::CTILE);
	const uint8 remapSource = 13; // Bright magenta
	const uint8 remapTarget = 0;  // Drawable black after masking

	if (isCTile) {
		debug("  - Loading with color remapping (CTILE mode)");
		_readyIcon = Pic::readWithRemapping(readyPicBlock, remapSource, remapTarget);
		_actionIcon = Pic::readWithRemapping(actionPicBlock, remapSource, remapTarget);
	} else {
		debug("  - Loading without remapping (PIC mode)");
		_readyIcon = Pic::read(readyPicBlock);
		_actionIcon = Pic::read(actionPicBlock);
	}

	if (!_readyIcon || !_actionIcon) {
		warning("Icon::Icon(blockId): Failed to create Pics readyIcon=%p actionIcon=%p", _readyIcon, _actionIcon);
		if (!_readyIcon) {
			int w = readyPicBlock ? (int)readyPicBlock->width : 24;
			int h = readyPicBlock ? (int)readyPicBlock->height : 24;
			_readyIcon = new Pic(w, h);
			_readyIcon->clear(0);
		}
		if (!_actionIcon) {
			int w = actionPicBlock ? (int)actionPicBlock->width : 24;
			int h = actionPicBlock ? (int)actionPicBlock->height : 24;
			_actionIcon = new Pic(w, h);
			_actionIcon->clear(0);
		}
	}

	debug("  - Created readyIcon=%p actionIcon=%p", _readyIcon, _actionIcon);

	// Build flipped versions
	_readyIconFlipped = createFlipped(_readyIcon);
	_actionIconFlipped = createFlipped(_actionIcon);
	debug("  - Created flipped variants: ready=%p action=%p", _readyIconFlipped, _actionIconFlipped);

	// Set current composite based on action state and orientation
	updateComposite();
	debug("  - Icon construction complete: composite=%p (state=%d, dir=%d)", _composite, _state, _direction);

	// Debug: Dump hex content of composite icons
	debugDumpPicHex(_readyIcon, "ReadyIcon (blockId constructor)");
	debugDumpPicHex(_actionIcon, "ActionIcon (blockId constructor)");
}

	Icon::Icon(const Data::CombatIconData &iconData,
	                             DaxRenderer *renderer,
	                             IconState state)
	    : _size(static_cast<IconSize>(iconData.iconSize)),
	      _state(state),
	      _direction(ICON_DIRECTION_RIGHT),
	      _renderer(renderer),
	      _composite(nullptr),
	      _readyIcon(nullptr),
	      _actionIcon(nullptr),
	      _readyIconFlipped(nullptr),
	      _actionIconFlipped(nullptr),
	      _readyHeadPic(nullptr),
	      _readyBodyPic(nullptr),
	      _actionHeadPic(nullptr),
	      _actionBodyPic(nullptr) {

		uint8 readyHeadId = resolveSpriteId(iconData.iconHead, _size);
		uint8 readyBodyId = resolveSpriteId(iconData.iconBody, _size);
		uint8 actionHeadId = static_cast<uint8>(readyHeadId + ACTION_OFFSET);
		uint8 actionBodyId = static_cast<uint8>(readyBodyId + ACTION_OFFSET);

		// Load base sprites via renderer if available
		if (!loadBaseSprites(readyHeadId, readyBodyId, actionHeadId, actionBodyId)) {
			_readyIcon = new Pic(24, 24);
			_readyIcon->clear(0);
			_actionIcon = new Pic(24, 24);
			_actionIcon->clear(0);
		} else {
			_readyIcon = buildComposite(false, iconData);
			_actionIcon = buildComposite(true, iconData);
		}

		// Build flipped versions
		_readyIconFlipped = createFlipped(_readyIcon);
		_actionIconFlipped = createFlipped(_actionIcon);

		// Set current composite based on action state and orientation
		updateComposite();
	}

Icon::~Icon() {
	delete _readyIcon;
	delete _actionIcon;
	delete _readyIconFlipped;
	delete _actionIconFlipped;
	// _composite is not deleted here as it points to either _readyIcon or _actionIcon
	_composite = nullptr;

	// Clean up cached base sprites
	delete _readyHeadPic;
	delete _readyBodyPic;
	delete _actionHeadPic;
	delete _actionBodyPic;
}

bool Icon::loadBaseSprites(uint8 readyHeadId, uint8 readyBodyId,
									uint8 actionHeadId, uint8 actionBodyId) {
	debug("Icon::loadBaseSprites ids readyHead=%u readyBody=%u actionHead=%u actionBody=%u renderer=%s",
			readyHeadId, readyBodyId, actionHeadId, actionBodyId, _renderer ? "yes" : "no");
			debug("  - Content types: head=%d body=%d",
			      (int)VmInterface::getDaxCHead().getContentType(),
			      (int)VmInterface::getDaxCBody().getContentType());
	if (_renderer) {
		_readyHeadPic = _renderer->readHead(readyHeadId);
		_readyBodyPic = _renderer->readBody(readyBodyId);
		_actionHeadPic = _renderer->readHead(actionHeadId);
		_actionBodyPic = _renderer->readBody(actionBodyId);

		if (!_readyHeadPic || !_readyBodyPic || !_actionHeadPic || !_actionBodyPic) {
			debug("Icon::loadBaseSprites renderer load failed headR=%p bodyR=%p headA=%p bodyA=%p",
					_readyHeadPic, _readyBodyPic, _actionHeadPic, _actionBodyPic);
			return false;
		}
		return true;
	}

	// Load ready state sprites from DAX containers
	Data::DaxBlockPic *readyHeadBlock = static_cast<Data::DaxBlockPic *>(
		VmInterface::getDaxCHead().getBlockById(readyHeadId));
	Data::DaxBlockPic *readyBodyBlock = static_cast<Data::DaxBlockPic *>(
		VmInterface::getDaxCBody().getBlockById(readyBodyId));

	if (!readyHeadBlock || !readyBodyBlock) {
		debug("Icon::loadBaseSprites missing ready blocks head=%p body=%p", readyHeadBlock, readyBodyBlock);
		return false;
	}

	// Load action state sprites from DAX containers
	Data::DaxBlockPic *actionHeadBlock = static_cast<Data::DaxBlockPic *>(
		VmInterface::getDaxCHead().getBlockById(actionHeadId));
	Data::DaxBlockPic *actionBodyBlock = static_cast<Data::DaxBlockPic *>(
		VmInterface::getDaxCBody().getBlockById(actionBodyId));

	if (!actionHeadBlock || !actionBodyBlock) {
		debug("Icon::loadBaseSprites missing action blocks head=%p body=%p", actionHeadBlock, actionBodyBlock);
		return false;
	}

	// Cache the base Pics (pre-made from DAX)
	const bool headIsCTile = VmInterface::getDaxCHead().getContentType() == Data::ContentType::CTILE;
	const bool bodyIsCTile = VmInterface::getDaxCBody().getContentType() == Data::ContentType::CTILE;
	const uint8 remapSource = 13; // Bright magenta
	const uint8 remapTarget = 0;  // Drawable black after masking

	_readyHeadPic = headIsCTile ? Pic::readWithRemapping(readyHeadBlock, remapSource, remapTarget) : Pic::read(readyHeadBlock);
	_readyBodyPic = bodyIsCTile ? Pic::readWithRemapping(readyBodyBlock, remapSource, remapTarget) : Pic::read(readyBodyBlock);
	_actionHeadPic = headIsCTile ? Pic::readWithRemapping(actionHeadBlock, remapSource, remapTarget) : Pic::read(actionHeadBlock);
	_actionBodyPic = bodyIsCTile ? Pic::readWithRemapping(actionBodyBlock, remapSource, remapTarget) : Pic::read(actionBodyBlock);

	return true;
}

Pic *Icon::buildComposite(bool isAction, const Data::CombatIconData &iconData) {
	// Use cached base sprites (already loaded from DAX)
	Pic *headPic = isAction ? _actionHeadPic : _readyHeadPic;
	Pic *bodyPic = isAction ? _actionBodyPic : _readyBodyPic;
	debug("Icon::buildComposite: isAction=%d headPic=%p bodyPic=%p", (int)isAction, headPic, bodyPic);

	if (!headPic || !bodyPic) {
		// Create empty placeholder if sprites missing
		Pic *placeholder = new Pic(24, 24);
		placeholder->clear(0);
		return placeholder;
	}

	// Build composite first from unmodified originals
	Pic *composite = bodyPic->clone();
	headPic->trDraw(composite, 0, 0, headPic->getTransparentIndex());

	// Now recolorize on the final composite so head/body act as one
	remapComposite(composite, iconData);

	return composite;
}

Pic *Icon::createFlipped(const Pic *source) const {
	if (!source) {
		return nullptr;
	}

	Pic *flipped = new Pic(source->w, source->h);

	// Mirror horizontally (left/right flip) for facing direction changes
	for (int y = 0; y < source->h; ++y) {
		const byte *srcRow = (const byte *)source->getBasePtr(0, y);
		byte *dstRow = (byte *)flipped->getBasePtr(0, y);
		for (int x = 0; x < source->w; ++x) {
			dstRow[x] = srcRow[source->w - 1 - x];
		}
	}

	// No mask to flip when using colorkey transparency

	return flipped;
}

void Icon::remapComposite(Pic *composite, const Data::CombatIconData &iconData) const {
	if (!composite)
		return;
	debug("Icon::remapComposite: size=%u colors BODY(%u,%u) ARM(%u,%u) LEG(%u,%u) SHIELD(%u,%u) WEAP(%u,%u) HAIR=%u FACE=%u",
	      (unsigned)iconData.iconSize,
	      (unsigned)iconData.iconColorBody1, (unsigned)iconData.iconColorBody2,
	      (unsigned)iconData.iconColorArm1, (unsigned)iconData.iconColorArm2,
	      (unsigned)iconData.iconColorLeg1, (unsigned)iconData.iconColorLeg2,
	      (unsigned)iconData.iconColorShield1, (unsigned)iconData.iconColorShield2,
	      (unsigned)iconData.iconColorWeapon1, (unsigned)iconData.iconColorWeapon2,
	      (unsigned)iconData.iconColorHair, (unsigned)iconData.iconColorFace);

	byte *pixels = (byte *)composite->getPixels();
	const int pixelCount = composite->w * composite->h;

	const uint8 BODY1   = iconData.iconColorBody1;
	const uint8 BODY2   = iconData.iconColorBody2;
	const uint8 ARM1    = iconData.iconColorArm1;
	const uint8 ARM2    = iconData.iconColorArm2;
	const uint8 LEG1    = iconData.iconColorLeg1;
	const uint8 LEG2    = iconData.iconColorLeg2;
	const uint8 SHIELD1 = iconData.iconColorShield1;
	const uint8 SHIELD2 = iconData.iconColorShield2;
	const uint8 WEAP1   = iconData.iconColorWeapon1;
	const uint8 WEAP2   = iconData.iconColorWeapon2;
	const uint8 HAIR    = iconData.iconColorHair;
	const uint8 FACE    = iconData.iconColorFace;

	for (int i = 0; i < pixelCount; ++i) {
		const uint8 src = pixels[i];
		switch (src) {
		case 1:  pixels[i] = BODY1;   break;
		case 9:  pixels[i] = BODY2;   break;
		case 2:  pixels[i] = ARM1;    break;
		case 10: pixels[i] = ARM2;    break;
		case 3:  pixels[i] = LEG1;    break;
		case 11: pixels[i] = LEG2;    break;
		case 6:  pixels[i] = SHIELD1; break;
		case 14: pixels[i] = SHIELD2; break;
		case 7:  pixels[i] = WEAP1;   break;
		case 15: pixels[i] = WEAP2;   break;
		case 4:  pixels[i] = HAIR;    break;
		case 12: pixels[i] = FACE;    break;
		default:
			break;
		}
	}
}

int Icon::getWidth() const {
	return _composite ? _composite->w : 0;
}

int Icon::getHeight() const {
	return _composite ? _composite->h : 0;
}

// Static utility functions
uint8 Icon::calculateSpriteId(uint8 baseId, uint8 size, bool isAction) {
	uint8 sizeOffset = (size == SIZE_LARGE) ? SIZE_OFFSET_LARGE : 0;
	uint8 actionOffset = isAction ? ACTION_OFFSET : 0;
	return baseId + sizeOffset + actionOffset;
}

uint8 Icon::extractBaseId(uint8 spriteId) {
	if (spriteId >= ACTION_OFFSET) {
		spriteId -= ACTION_OFFSET;
	}
	if (spriteId >= SIZE_OFFSET_LARGE) {
		spriteId -= SIZE_OFFSET_LARGE;
	}
	return spriteId;
}

bool Icon::isValidHeadBaseId(uint8 baseId) {
	return baseId <= ICON_HEAD_MAX;
}

bool Icon::isValidBodyBaseId(uint8 baseId) {
	return baseId <= ICON_BODY_MAX;
}

uint8 Icon::nextHead(uint8 currentBaseId) {
	return (currentBaseId >= ICON_HEAD_MAX) ? 0 : static_cast<uint8>(currentBaseId + 1);
}

uint8 Icon::prevHead(uint8 currentBaseId) {
	return (currentBaseId == 0) ? ICON_HEAD_MAX : static_cast<uint8>(currentBaseId - 1);
}

uint8 Icon::nextBody(uint8 currentBaseId) {
	return (currentBaseId >= ICON_BODY_MAX) ? 0 : static_cast<uint8>(currentBaseId + 1);
}

uint8 Icon::prevBody(uint8 currentBaseId) {
	return (currentBaseId == 0) ? ICON_BODY_MAX : static_cast<uint8>(currentBaseId - 1);
}

uint8 Icon::nextColor(uint8 currentColor) {
	return (currentColor + 1) & 0x0F;
}

uint8 Icon::prevColor(uint8 currentColor) {
	return (currentColor == 0) ? COLOR_MAX : currentColor - 1;
}

void Icon::getDimensions(uint8 size, int &width, int &height) {
	// Derive dimensions from DAX body sprite metadata for the requested size.
	const uint8 sizeOffset = (size == SIZE_LARGE) ? SIZE_OFFSET_LARGE : 0;
	Data::DaxBlockPic *bodyBlock = static_cast<Data::DaxBlockPic *>(
		VmInterface::getDaxCBody().getBlockById(0 + sizeOffset));
	if (bodyBlock) {
		width = (int)bodyBlock->width;
		height = (int)bodyBlock->height;
	} else {
		// Fallback to a minimal safe default if metadata unavailable
		width = 24;
		height = 24;
	}
}

void Icon::setActionState(IconState state) {
	if (_state != state) {
		_state = state;
		updateComposite();
	}
}

void Icon::setOrientation(IconDirection direction) {
	if (_direction != direction) {
		_direction = direction;
		updateComposite();
	}
}

void Icon::toggleOrientation() {
	_direction = (_direction == ICON_DIRECTION_RIGHT) ? ICON_DIRECTION_LEFT : ICON_DIRECTION_RIGHT;
	updateComposite();
}

void Icon::drawAtIconPos(Graphics::ManagedSurface *surface, int iconX, int iconY) const {
	if (!_composite || !surface) {
		return;
	}

	// Convert icon coordinates to character tile coordinates (3x3 char tiles per icon, +1 offset)
	int charX = iconX * 3 + 1;
	int charY = iconY * 3 + 1;

	// Convert character coordinates to pixels (8x8 per char)
	int pixelX = charX * 8;
	int pixelY = charY * 8;

	// Draw using the composite's transparent index (0 for PIC, 255 for CTILE)
	_composite->trDraw(surface, pixelX, pixelY, _composite->getTransparentIndex());
}

void Icon::debugDumpPicHex(const Pic *pic, const char *label) const {
	if (!pic) {
		debug("debugDumpPicHex: %s - Pic is NULL", label);
		return;
	}

	int width = pic->w;
	int height = pic->h;
	debug("=== debugDumpPicHex: %s ===", label);
	debug("Dimensions: %d x %d", width, height);
	debug("Format: 8-bit indexed color (palette)");
	debug("---");

	// Get the pixel data
	byte *pixels = (byte *)pic->getPixels();
	if (!pixels) {
		debug("  ERROR: Failed to get pixel data");
		return;
	}

	// Get pitch (bytes per row, may include padding)
	int pitch = pic->pitch;
	debug("Pitch: %d bytes", pitch);
	debug("---");

	// Dump pixel data row by row
	for (int y = 0; y < height; y++) {
		Common::String hexLine;
		hexLine += Common::String::format("Row %2d: ", y);

		byte *row = pixels + (y * pitch);
		for (int x = 0; x < width; x++) {
			byte pixel = row[x];
			hexLine += Common::String::format("%02X ", pixel);
		}

		debug("%s", hexLine.c_str());
	}

	// Also dump transparency mask if available
	Common::BitArray *mask = pic->getTransparencyMask();
	if (mask) {
		debug("---");
		debug("Transparency Mask (1=transparent, 0=opaque):");
		for (int y = 0; y < height; y++) {
			Common::String maskLine;
			maskLine += Common::String::format("Row %2d: ", y);

			for (int x = 0; x < width; x++) {
				int bitIndex = y * width + x;
				bool isTransparent = mask->get(bitIndex);
				maskLine += Common::String::format("%d ", isTransparent ? 1 : 0);
			}

			debug("%s", maskLine.c_str());
		}
	}

	debug("Transparent index: %u", pic->getTransparentIndex());
	debug("===\n");
}

} // namespace Gfx
} // namespace Goldbox

