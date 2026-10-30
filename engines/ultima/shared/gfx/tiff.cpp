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

#include "ultima/shared/gfx/tiff.h"
#include "common/util.h"

namespace Image {
namespace Ultima {
namespace Shared {
namespace Gfx {


TIFFDecoder::TIFFDecoder() {
	_surface = 0;

	destroy();
}

TIFFDecoder::~TIFFDecoder() {
	destroy();
}

void TIFFDecoder::destroy() {
	if (_surface) {
		_surface->free();
		delete _surface;
		_surface = 0;
	}

	_palette.clear();

	_tags.clear();
	_bits_per_pixel = TIFF_NOTSET;
	_photometric_int = 255;
	_width = _height = 0;
	_offset = _stripByteCount = 0;
	_fill_order = 1;
}

bool TIFFDecoder::loadStream(Common::SeekableReadStream &stream) {
	destroy();

	if (!loadHeader(stream) || !processImageFileDirectory() || !loadBitmap(stream)) {
		destroy();
		return false;
	}
	return true;
}

bool TIFFDecoder::loadHeader(Common::SeekableReadStream &stream) {
	if (stream.size() < 8 || !stream.seek(0)) {
		warning("Invalid FM Towns TIFF header");
		return false;
	}

	bool bigEndian;
	switch (stream.readUint16BE()) {
	case ID_II:
		bigEndian = false;
		break;
	case ID_MM:
		bigEndian = true;
		break;
	default:
		warning("Invalid FM Towns TIFF byte order");
		return false;
	}

	Common::SeekableReadStreamEndianWrapper reader(&stream, bigEndian);
	const uint16 version = reader.readUint16();
	const uint32 directoryOffset = reader.readUint32();
	if (version != VERSION || directoryOffset < 8 ||
			directoryOffset > stream.size() - 2 ||
			!stream.seek(directoryOffset)) {
		warning("Invalid FM Towns TIFF directory offset or version");
		return false;
	}

	const uint16 tagCount = reader.readUint16();
	if (tagCount > 4096 || stream.size() - stream.pos() < tagCount * 12 + 4) {
		warning("Invalid or oversized FM Towns TIFF directory");
		return false;
	}

	_tags.resize(tagCount);
	for (uint16 ifd = 0; ifd < tagCount; ++ifd) {
		IFDEntry &entry = _tags[ifd];
		entry.tag = reader.readUint16();
		entry.type = reader.readUint16();
		entry.count = reader.readUint32();
		entry.value = reader.readUint32();
		if (entry.type == TIFF_SHORT && entry.count == 1)
			entry.value = bigEndian ? entry.value >> 16 : entry.value & 0xffff;
	}
	if (stream.err() || stream.eos()) {
		warning("Error reading FM Towns TIFF directory");
		return false;
	}
	return true;
}

bool TIFFDecoder::processImageFileDirectory() {
	for (const IFDEntry &entry : _tags) {
		switch (entry.tag) {
		case TIFFTAG_IMAGEWIDTH:
		case TIFFTAG_IMAGELENGTH:
		case TIFFTAG_BITSPERSAMPLE:
		case TIFFTAG_COMPRESSION:
		case TIFFTAG_PHOTOMETRIC:
		case TIFFTAG_SAMPLESPERPIXEL:
		case TIFFTAG_FILLORDER:
		case TIFFTAG_STRIPOFFSETS:
		case TIFFTAG_STRIPBYTECOUNTS:
			if (entry.count != 1 || (entry.type != TIFF_SHORT && entry.type != TIFF_LONG)) {
				warning("Unsupported FM Towns TIFF tag %u", entry.tag);
				return false;
			}
			break;
		default:
			continue;
		}

		switch (entry.tag) {
		case TIFFTAG_IMAGEWIDTH:
		case TIFFTAG_IMAGELENGTH:
			if (!entry.value || entry.value > 0x7fff) {
				warning("Invalid FM Towns TIFF dimensions");
				return false;
			}
			if (entry.tag == TIFFTAG_IMAGEWIDTH)
				_width = entry.value;
			else
				_height = entry.value;
			break;
		case TIFFTAG_BITSPERSAMPLE:
			if (entry.value != TIFF_16 && entry.value != TIFF_256 && entry.value != TIFF_32K) {
				warning("Unsupported FM Towns TIFF bits per sample");
				return false;
			}
			_bits_per_pixel = entry.value;
			break;
		case TIFFTAG_COMPRESSION:
			if (entry.value != TIFF_COMPRESSION_NONE) {
				warning("Unsupported FM Towns TIFF compression");
				return false;
			}
			break;
		case TIFFTAG_PHOTOMETRIC:
			if (entry.value > 3) {
				warning("Unsupported FM Towns TIFF photometric interpretation");
				return false;
			}
			_photometric_int = entry.value;
			break;
		case TIFFTAG_SAMPLESPERPIXEL:
			if (entry.value != 1) {
				warning("Unsupported FM Towns TIFF samples per pixel");
				return false;
			}
			break;
		case TIFFTAG_FILLORDER:
			if (entry.value != 1 && entry.value != 2) {
				warning("Unsupported FM Towns TIFF fill order");
				return false;
			}
			_fill_order = entry.value;
			break;
		case TIFFTAG_STRIPOFFSETS:
			_offset = entry.value;
			break;
		case TIFFTAG_STRIPBYTECOUNTS:
			_stripByteCount = entry.value;
			break;
		}
	}

	if (!_width || !_height || !_offset || _bits_per_pixel == TIFF_NOTSET ||
			(_fill_order == 2 && _bits_per_pixel != TIFF_16) ||
			(_bits_per_pixel == TIFF_32K && _photometric_int != 1)) {
		warning("Invalid or unsupported FM Towns TIFF image description");
		return false;
	}
	return true;
}

bool TIFFDecoder::loadBitmap(Common::SeekableReadStream &stream) {
	const uint32 rowBytes = (uint32(_width) * _bits_per_pixel + 7) / 8;
	const uint32 dataLength = rowBytes * _height;
	const uint32 decodedSize = uint32(_width) * _height * (_bits_per_pixel == TIFF_32K ? 2 : 1);
	if (decodedSize > 64 * 1024 * 1024) {
		warning("FM Towns TIFF exceeds the 64 MiB decoded-image limit");
		return false;
	}
	if (_offset > stream.size() || dataLength > stream.size() - _offset ||
			(_stripByteCount && (_stripByteCount < dataLength || _stripByteCount > stream.size() - _offset)) ||
			!stream.seek(_offset)) {
		warning("Truncated FM Towns TIFF pixel data");
		return false;
	}

	_surface = new Graphics::Surface();
	_surface->create(_width, _height, _bits_per_pixel == TIFF_32K ?
		Graphics::PixelFormat(2, 5, 5, 5, 0, 5, 10, 0, 0) :
		Graphics::PixelFormat::createFormatCLUT8());

	for (uint16 y = 0; y < _height; ++y) {
		if (_bits_per_pixel == TIFF_32K) {
			uint16 *dest = static_cast<uint16 *>(_surface->getBasePtr(0, y));
			for (uint16 x = 0; x < _width; ++x)
				dest[x] = stream.readUint16LE();
		} else {
			byte *dest = static_cast<byte *>(_surface->getBasePtr(0, y));
			if (_bits_per_pixel == TIFF_16) {
				for (uint16 x = 0; x < _width; x += 2) {
					const byte packedPixel = stream.readByte();
					dest[x] = packedPixel & 0x0f;
					if (x + 1 < _width)
						dest[x + 1] = packedPixel >> 4;
				}
			} else if (stream.read(dest, rowBytes) != rowBytes) {
				warning("Error reading FM Towns TIFF pixels");
				return false;
			}
		}
	}
	if (stream.err() || stream.eos()) {
		warning("Error reading FM Towns TIFF pixels");
		return false;
	}
	return true;
}

} // End of namespace Gfx 
} // End of namespace Shared
} // End of namespace Ultima 
} // End of namespace Image
