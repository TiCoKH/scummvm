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
#include "ultima/shared/core/file.h"
#include "common/util.h"
#include "common/memstream.h"

namespace Image {
namespace Ultima {
namespace Shared {
namespace Gfx {


TIFFDecoder::TIFFDecoder() {
	_surface = 0;
	_palette = 0;

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

	if (_palette) {
		delete[] _palette;
		_palette = 0;
	}

	memset(&_header, 0, sizeof(TownsHeader));
	_byte_order = TIFFBO_UNKNOWN;
	_bits_per_pixel = TIFF_NOTSET;
	_sample_per_pixel = 0;
	_paletteColorCount = 0;
	_photometric_int = 255;
	_compression = TIFF_COMPRESSION_NOTSET;
}

bool TIFFDecoder::loadStream(Common::SeekableReadStream &stream) {
	destroy();

	loadHeader(stream);
	processImageFileDirectory();
	loadBitmap(stream);

	while (1) {
		if (stream.size() < stream.pos() + 8)
			break;
		if (stream.eos())
			break;
	}

	return true;
}

void TIFFDecoder::loadHeader(Common::SeekableReadStream &stream) {
	_header.head.tiff_magic = stream.readUint16BE();
	_header.head.tiff_version = stream.readUint16BE();
	if (_header.head.tiff_version == 42){
		switch (_header.head.tiff_magic)
		{
		case ID_II:
			_byte_order = TIFFBO_BIGENDIAN;
			break;
		case ID_MM:
			_byte_order = TIFFBO_LITTLEENDIAN;
			break;
		default:
			warning("Unknown byteordered TIFF-file, force to Intel type");
			_byte_order = TIFFBO_BIGENDIAN;
			break;
		}
		warning("Unknown version TIFF-file, force to only possible value");
		_header.head.tiff_version = 42;
	}

	if (_byte_order == TIFFBO_BIGENDIAN) {
		_header.head.tiff_diroff = stream.readUint32BE();
		_header.ifdnum = stream.readUint16BE();
	}
	else {
		_header.head.tiff_diroff = stream.readUint32LE();
		_header.ifdnum = stream.readUint16BE();
	}

	if (_header.head.tiff_diroff != stream.pos()){
		stream.seek( _header.head.tiff_diroff, SEEK_SET );
	}

	for (short ifd = 0; ifd < _header.ifdnum; ifd++) {
		if (_byte_order == TIFFBO_BIGENDIAN) {
			_header.tags[ifd].tag = stream.readUint16BE();
			_header.tags[ifd].type = stream.readUint16BE();
			_header.tags[ifd].count = stream.readUint32BE();
			_header.tags[ifd].value = stream.readUint32BE();
		}
		else {
			_header.tags[ifd].tag = stream.readUint16LE();
			_header.tags[ifd].type = stream.readUint16LE();
			_header.tags[ifd].count = stream.readUint32LE();
			_header.tags[ifd].value = stream.readUint32LE();
		}
		if (ifd == 15) {
			warning("Current TIFF-file reader support only 16 IFD Entry (FMTowns default), futher tags will be dropped");
			break;
		}
	}
}

void  TIFFDecoder::processImageFileDirectory() {

	for (short ifd = 0; ifd < _header.ifdnum; ifd++) {

		switch (_header.tags[ifd].tag) {
			case TIFFTAG_NEWSUBFILETYPE:
				break;
			case TIFFTAG_SUBFILETYPE:
				break;
			case TIFFTAG_IMAGEWIDTH:
				_width = _header.tags[ifd].value;
				break;
			case TIFFTAG_IMAGELENGTH:
				_height = _header.tags[ifd].value;
				break;
			case TIFFTAG_BITSPERSAMPLE:
				setBitsPerPixel( _header.tags[ifd].value );
				break;
			case TIFFTAG_COMPRESSION:
				_compression = _header.tags[ifd].value;
				if (_compression != (TIFF_COMPRESSION_NONE || TIFF_COMPRESSION_LZW)){
					warning("Unknown or unsupported compression");
				}
				break;
			case TIFFTAG_FILLORDER:
			 _fill_order = _header.tags[ifd].value;
			 if (_fill_order != (1 || 2)){
					warning("Unknown fill order");
				}
				break;
			case TIFFTAG_STRIPOFFSETS:
				_offset = _header.tags[ifd].value;
				break;
			case TIFFTAG_ORIENTATION:
				// TODO, not handled now 
				break;
			case TIFFTAG_MINSAMPLEVALUE:
				_min_sample = _header.tags[ifd].value;
				break;
			case TIFFTAG_MAXSAMPLEVALUE:
				_max_sample = _header.tags[ifd].value;
				break;
			case TIFFTAG_XRESOLUTION:
				// TODO, not handled now
				break;
			case TIFFTAG_YRESOLUTION:
				// TODO, not handled now
				break;
			case TIFFTAG_PLANARCONFIG:
				// TODO, not handled now
				break;

		}
	}
}

void TIFFDecoder::setBitsPerPixel( long value ) {
	
	for (short ifd = 0; ifd < _header.ifdnum; ifd++) {
		if (_header.tags[ifd].tag == TIFFTAG_SAMPLESPERPIXEL ){
			_sample_per_pixel = _header.tags[ifd].value;
		}
		if (_header.tags[ifd].tag == TIFFTAG_PHOTOMETRIC ){
			_photometric_int = _header.tags[ifd].value;
		}
	}
	switch (value)
	{
	case 1:
		_bits_per_pixel = TIFF_MONO;
		if (_sample_per_pixel != 1){
			warning("Bilevel image SamplesPerPixel value is must be 1");
			_sample_per_pixel = 1;
		}
		if (_photometric_int != 0){
			warning("Bilevel image PhotometricInterpretation value is must be 1");
			_photometric_int = 0;
		}
		break;
	case TIFF_16:
		_bits_per_pixel = TIFF_16;
		if (_sample_per_pixel != 1){
			warning("4bit palette image SamplesPerPixel value is must be 1");
			_sample_per_pixel = 1;
		}
		break;
	case TIFF_32K:
		_bits_per_pixel = TIFF_32K;
		if (_sample_per_pixel != 1){
			warning("16bit palette image SamplesPerPixel value is must be 1");
			_sample_per_pixel = 1;
		}
		break;
	default:
		warning("Unhandled BitPerPixel value");
		break;
	}
}

void TIFFDecoder::loadBitmap(Common::SeekableReadStream &stream) {

	stream.seek( _offset, SEEK_SET );

	_surface = new Graphics::Surface();
	uint32 data_lenght;

	uint32 outSize = _width * _height;
	byte *data;
	
	switch (_bits_per_pixel)
	{
	case TIFF_16:
		_surface->create( _width, _height, Graphics::PixelFormat::createFormatCLUT8() );
		data = (byte *)_surface->getPixels();
		byte packedpixel;
		data_lenght = ((_width + 1) / 2) * _height;
		for(int i = 0; i < data_lenght; i++) {
			packedpixel = stream.readByte();
			*data++ = packedpixel & 0x0F;
			*data++ = packedpixel >> 4;
		}
		break;

	case TIFF_256:
		_surface->create( _width, _height, Graphics::PixelFormat::createFormatCLUT8() );
		data = (byte *)_surface->getPixels();
		data_lenght = _width * _height;
		data = (byte *)stream.readStream(data_lenght);
		/*
		for(int i = 0; i <= data_lenght; i++) {
			*data++ = stream.readByte();
		}
		*/
		break;
 
	case TIFF_32K:
		_surface->create( _width, _height, Graphics::PixelFormat(2, 5, 5, 5, 0, 5, 10, 0, 0) );
		data = (byte *)_surface->getPixels();
		data_lenght = _width * _height * 2;
		data = (byte *)stream.readStream(data_lenght);
		break;
	default:
		warning("Unsupported pit per pixel");

	}

}

} // End of namespace Gfx 
} // End of namespace Shared
} // End of namespace Ultima 
} // End of namespace Image
