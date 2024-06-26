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

#include "common/config-manager.h"
#include "common/formats/winexe_pe.h"

#include "scumm/he/intern_he.h"
#include "scumm/he/moonbase/moonbase.h"

namespace Scumm {

#define FOW_ANIM_FRAME_COUNT	38

void Moonbase::initFOW() {
	_fowSentinelImage = -1;
	_fowSentinelState = -1;
	_fowSentinelConditionBits = 0;

	_fowFrameBaseNumber = 0;
	_fowAnimationFrames = 1;
	_fowCurrentFOWFrame = 0;

	_fowTileW = 0;
	_fowTileH = 0;

	_fowImage = nullptr;
	_fowClipX1 = 0;
	_fowClipY1 = 0;
	_fowClipX2 = 0;
	_fowClipY2 = 0;

	_fowDrawX = 0;
	_fowDrawY = 0;

	_fowVtx1 = 0;
	_fowVty1 = 0;
	_fowMvx = 0;
	_fowMvy = 0;
	_fowVw = 0;
	_fowVh = 0;

	_fowBlackMode = true;

	memset(_fowRenderTable, 0, sizeof(_fowRenderTable));
}

void Moonbase::releaseFOWResources() {
	if (_fowImage) {
		free(_fowImage);
		_fowImage = nullptr;
	}
}

bool Moonbase::setFOWImage(int image) {
	releaseFOWResources();

	if (!_fowImage) {
		Common::String fowImageFilename(ConfMan.get("MOONX_FOWImageFilename").c_str());

#if 0 // TODO
		if (!fowImageFilename.empty()) {
			void *wiz = loadWizFromFilename(fowImageFilename);

			if (wiz) {
				captureFOWImageFromLocation(wiz, file.size());
				free(wiz);
			}
		}
#endif

		if (!_fowImage && image < 0) {
			int resId;

			// PIECES  BUBBLES CIRCLES SIMPLE*  WEDGEY BUBBLE2
			// WEDGE2  SPIKEY  ANGLES  SMOOTHED WUZZY  SYS7-BEVELED
			if (image >= -12 && image <= -1)
				resId = 210 - image; // 211-222 range
			else
				resId = 214; // default, SIMPLE

			if (_fileName.empty()) { // We are running for the first time
				_fileName = _vm->generateFilename(-3);

				if (!_exe->loadFromEXE(_fileName))
					error("Cannot open file %s", _fileName.toString(Common::Path::kNativeSeparator).c_str());
			}

			Common::SeekableReadStream *stream = _exe->getResource(Common::kWinRCData, resId);

			if (stream->size()) {
				_fowImage = (uint8 *)malloc(stream->size());

				stream->read(_fowImage, stream->size());
			}

			delete stream;
		}

		if (!_fowImage && image > 0) {
			int sz = _vm->getResourceSize(rtImage, image);
			_fowImage = (uint8 *)malloc(sz);

			// We have to copy it, otherwise the resource manager
			// will kill it earlier or later. Matches original.
			memcpy(_fowImage, _vm->getResourceAddress(rtImage, image), sz);
		}

		if (!_fowImage)
			return false;
	}

	int nStates = _vm->_wiz->getWizImageStates(_fowImage);

	if (nStates > FOW_ANIM_FRAME_COUNT) {
		releaseFOWResources();
		return false;
	}

	_fowAnimationFrames = (nStates + FOW_ANIM_FRAME_COUNT - 1) / FOW_ANIM_FRAME_COUNT;

	_vm->_wiz->getWizImageDim(_fowImage, (nStates - 1), _fowTileW, _fowTileH);

	bool sizeIndicatorWasBlack = true;
	int32 hitTestValue = 0;
	int32 pixelValue = 0;

	_vm->_wiz->moonbaseLayeredWizHitTest(
		&hitTestValue, &pixelValue, _fowImage, (nStates - 1), 0, 0, 0, 0);

	sizeIndicatorWasBlack = (pixelValue == 0);

	if (ConfMan.hasKey("EnableFOWRects"))
		_fowBlackMode = (ConfMan.getInt("EnableFOWRects") == 1);
	else
		_fowBlackMode = sizeIndicatorWasBlack;

	return true;
}

enum FOWElement {
	FOW_EMPTY	= 0,
	FOW_SOLID	= 1,

	FF_L		= 0x01,
	FF_R		= 0x02,
	FF_T		= 0x04,
	FF_B		= 0x08,
	FF_T_L		= 0x10,
	FF_T_R		= 0x20,
	FF_B_L		= 0x40,
	FF_B_R		= 0x80,
	FF_Q_A		= (FF_L | FF_T | FF_T_L),
	FF_Q_B		= (FF_R | FF_T | FF_T_R),
	FF_Q_C		= (FF_L | FF_B | FF_B_L),
	FF_Q_D		= (FF_R | FF_B | FF_B_R)
};

int Moonbase::readFOWVisibilityArray(int array, int y, int x) {
	if (readFromArray(array, x, y) > 0)
		return FOW_EMPTY;

	return FOW_SOLID;
}

void Moonbase::setFOWInfo(int fowInfoArray, int downDim, int acrossDim, int viewX, int viewY, int clipX1,
				int clipY1, int clipX2, int clipY2, int technique, int nFrame) {
	if (!_fowImage)
		return;

	memset(_fowRenderTable, 0, sizeof(_fowRenderTable));

	_fowDrawX = clipX1;
	_fowDrawY = clipY1;

	_fowClipX1 = clipX1;
	_fowClipY1 = clipY1;
	_fowClipX2 = clipX2;
	_fowClipY2 = clipY2;

	// Figure out the number of tiles are involved
	int view_W = (clipX2 - clipX1) + 1;
	int view_H = (clipY2 - clipY1) + 1;

	int tw = _fowTileW;
	int th = _fowTileH;

	int dw = acrossDim;
	int dh = downDim;

	int dlw = dw * tw;
	int dlh = dh * th;

	_fowMvx = (0 <= viewX) ? (viewX % dlw) : (dlw - (-viewX % dlw));
	_fowMvy = (0 <= viewY) ? (viewY % dlh) : (dlh - (-viewY % dlh));

	_fowVtx1 = _fowMvx / tw;
	_fowVty1 = _fowMvy / th;

	_fowVw = (((_fowMvx + view_W + tw - 1) / tw) - _fowVtx1) + 1;
	_fowVh = (((_fowMvy + view_H + th - 1) / th) - _fowVty1) + 1;

	// Build the connectivity table
	int t = (_fowVty1 - 1); if (t >= dh) { t = 0; } else if (t < 0) { t = (dh - 1); }
	int m = (_fowVty1 + 0); if (m >= dh) { m = 0; } else if (m < 0) { m = (dh - 1); }
	int b = (_fowVty1 + 1); if (b >= dh) { b = 0; } else if (b < 0) { b = (dh - 1); }

	int il = (_fowVtx1 - 1); if (il >= dh) { il = 0; } else if (il < 0) { il = (dw - 1); }
	int ic = (_fowVtx1 + 0); if (ic >= dh) { ic = 0; } else if (ic < 0) { ic = (dw - 1); }
	int ir = (_fowVtx1 + 1); if (ir >= dh) { ir = 0; } else if (ir < 0) { ir = (dw - 1); }

	int dataOffset = (_fowVw * 3);
	int dataOffset2 = (dataOffset * 2);
	int32 *pOutterRenderTableA = _fowRenderTable;
	int32 *pOutterRenderTableB = pOutterRenderTableA + dataOffset;

	for (int ay = 0; ay < _fowVh; ay++) {
		int l = il;
		int c = ic;
		int r = ir;

		int32 *pRenderTableA = pOutterRenderTableA;
		int32 *pRenderTableB = pOutterRenderTableB;

		pOutterRenderTableA += dataOffset2;
		pOutterRenderTableB += dataOffset2;

		for (int ax = 0; ax < _fowVw; ax++) {
			int visibility = readFOWVisibilityArray(fowInfoArray, m, c);

			if (visibility == FOW_EMPTY) {
				uint32 bits = 0;

				if (readFOWVisibilityArray(fowInfoArray, t, l) != 0) bits |= FF_T_L;
				if (readFOWVisibilityArray(fowInfoArray, t, c) != 0) bits |= FF_T;
				if (readFOWVisibilityArray(fowInfoArray, t, r) != 0) bits |= FF_T_R;
				if (readFOWVisibilityArray(fowInfoArray, m, l) != 0) bits |= FF_L;
				if (readFOWVisibilityArray(fowInfoArray, m, r) != 0) bits |= FF_R;
				if (readFOWVisibilityArray(fowInfoArray, b, l) != 0) bits |= FF_B_L;
				if (readFOWVisibilityArray(fowInfoArray, b, c) != 0) bits |= FF_B;
				if (readFOWVisibilityArray(fowInfoArray, b, r) != 0) bits |= FF_B_R;

				if (bits) {
					*pRenderTableA++ = 1;
					*pRenderTableB++ = 1;

					// Quadrant (A)
					if (bits & FF_Q_A) {
						*pRenderTableA++ = (
							((FF_L   & bits) ? 1 : 0) |
							((FF_T   & bits) ? 2 : 0) |
							((FF_T_L & bits) ? 4 : 0)
						) + 0;
					} else {
						*pRenderTableA++ = 0;
					}

					// Quadrant (B)
					if (bits & FF_Q_B) {
						*pRenderTableA++ = (
							((FF_R   & bits) ? 1 : 0) |
							((FF_T   & bits) ? 2 : 0) |
							((FF_T_R & bits) ? 4 : 0)
						) + 8;
					} else {
						*pRenderTableA++ = 0;
					}

					// Quadrant (C)
					if (bits & FF_Q_C) {
						*pRenderTableB++ = (
							((FF_L   & bits) ? 1 : 0) |
							((FF_B   & bits) ? 2 : 0) |
							((FF_B_L & bits) ? 4 : 0)
						) + 16;
					} else {
						*pRenderTableB++ = 0;
					}

					// Quadrant (D)
					if (bits & FF_Q_D) {
						*pRenderTableB++ = (
							((FF_R   & bits) ? 1 : 0) |
							((FF_B   & bits) ? 2 : 0) |
							((FF_B_R & bits) ? 4 : 0)
						) + 24;
					} else {
						*pRenderTableB++ = 0;
					}
				} else {
					*pRenderTableA++ = 0;
					*pRenderTableB++ = 0;
				}
			} else {
				if (_fowBlackMode) {
					*pRenderTableA++ = 2;
					*pRenderTableB++ = 2;
				} else {
					*pRenderTableA++ = 1;
					*pRenderTableA++ = 33;
					*pRenderTableA++ = 34;

					*pRenderTableB++ = 1;
					*pRenderTableB++ = 35;
					*pRenderTableB++ = 36;
				}
			}

			if (++l >= dw) { l = 0; }
			if (++c >= dw) { c = 0; }
			if (++r >= dw) { r = 0; }
		}

		if (++t >= dh) { t = 0; }
		if (++m >= dh) { m = 0; }
		if (++b >= dh) { b = 0; }
	}

	_fowCurrentFOWFrame = (nFrame >= 0) ? (nFrame % _fowAnimationFrames) : ((-nFrame) % _fowAnimationFrames);
	_fowFrameBaseNumber = (_fowCurrentFOWFrame * FOW_ANIM_FRAME_COUNT);
}

void Moonbase::renderFOWState(WizMultiTypeBitmap *destSurface,int x, int y, int state) {
	int32 spotx, spoty;

	_vm->_wiz->getWizImageSpot(_fowImage, state, spotx, spoty);

	_vm->_wiz->drawMoonbaseLayeredWiz(
		destSurface->data,
		destSurface->width, destSurface->height,
		destSurface->stride, destSurface->format, destSurface->bpp,
		_fowImage, x - spotx, y - spoty, state,
		_fowClipX1, _fowClipY1, _fowClipX2, _fowClipY2,
		0, 0, nullptr, nullptr);
}

static void blackRect_16bpp(WizMultiTypeBitmap *destSurface, int x1, int y1, int x2, int y2) {
	byte *dst = destSurface->data + (destSurface->stride * y1) + (x1 * sizeof(WizRawPixel16));
	int h = (y2 - y1) + 1;
	int writeBytes = ((x2 - x1) + 1) * sizeof(WizRawPixel16);

	while (--h >= 0) {
		memset(dst, 0, writeBytes);
		dst += destSurface->stride;
	}
}

void Moonbase::renderFOW(WizMultiTypeBitmap *destSurface) {
	if (!_fowImage)
		return;

	const int32 *outerRenderTable = _fowRenderTable;
	int ixPos = ((_fowVtx1 * _fowTileW) - _fowMvx) + _fowDrawX;
	int yPos = ((_fowVty1 * _fowTileH) - _fowMvy) + _fowDrawY;
	int dataOffset = _fowVw * 3;
	int halfTileHeight = _fowTileH / 2;
	int cx2 = MIN<int>(_fowClipX2, (int)(destSurface->width - 1));
	int cy2 = MIN<int>(_fowClipY2, (int)(destSurface->height - 1));

	for (int ry = 0; ry < _fowVh; ry++) {
		int realYPos = yPos;

		for (int i = 0; i < 2; i++) {
			const int32 *renderTable = outerRenderTable;
			outerRenderTable += dataOffset;

			int xPos = ixPos;

			for (int rx = 0; rx < _fowVw; rx++) {
				int state = *renderTable++;
	
				if (state != 0) {
					if (state == 2) {
						int countLeft = (_fowVw - rx);
						int count = 0;
	
						for (; count < countLeft; count++) {
							if (*(renderTable + count) != 2)
								break;

							renderTable++;
							rx++;
						}
						count++;

						int x1 = xPos;
						int y1 = realYPos;

						xPos += _fowTileW * count;
						int x2 = (xPos - 1);
						int y2 = ((y1 + halfTileHeight) - 1);

						x1 = MAX(0, x1);
						y1 = MAX(0, y1);
						x2 = MIN(x2, cx2);
						y2 = MIN(y2, cy2);

						if ((x2 >= x1) && (y2 >= y1) && (x1 <= _fowClipX2) && (y1 <= _fowClipY2))
							blackRect_16bpp(destSurface, x1, y1, x2, y2);
					} else {
						int subState;

						if ((subState = *renderTable++) != 0)
							renderFOWState(destSurface, xPos, yPos, (subState + _fowFrameBaseNumber));

						if ((subState = *renderTable++) != 0)
							renderFOWState(destSurface, xPos, yPos, (subState + _fowFrameBaseNumber));

						xPos += _fowTileW;
					}
				} else {
					xPos += _fowTileW;
				}
			}

			realYPos += halfTileHeight;
		}

		yPos += _fowTileH;
	}
}

} // End of namespace Scumm
