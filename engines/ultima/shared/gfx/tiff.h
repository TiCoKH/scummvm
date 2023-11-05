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

#ifndef ULTIMA_SHARED_GFX_TIFF_H
#define ULTIMA_SHARED_GFX_TIFF_H

#include "common/array.h"
#include "common/endian.h"
#include "common/stream.h"
#include "graphics/surface.h"

#include "common/scummsys.h"
#include "common/str.h"
#include "image/image_decoder.h"

#include "ultima/shared/std/containers.h"

/*-------------------------*/
/* TIFF format definition  */
/*-------------------------*/
#define ID_II	0x4949
#define ID_MM	0x4d4d
#define VERSION	0x2A

/*-------------------------*/
/* for towns  color modes  */
/*-------------------------*/
#define TIFF_NOTSET	0
#define TIFF_MONO	1
#define TIFF_16		4
#define TIFF_256	8
#define TIFF_32K	16
#define TIFF_RGB	24

#define TIFF_COMPRESSION_NOTSET	0
#define TIFF_COMPRESSION_NONE	1
#define TIFF_COMPRESSION_LZW	5


#define TIFF_NOTYPE		0	/* placeholder */
#define TIFF_BYTE		1	/* 8-bit unsigned integer */
#define TIFF_ASCII		2	/* 8-bit bytes w/ last byte null */
#define TIFF_SHORT		3	/* 16-bit unsigned integer */
#define TIFF_LONG		4	/* 32-bit unsigned integer */
#define TIFF_RATIONAL	5	/* 64-bit unsigned fraction */


/*-------------------------*/
/* FM TOWNS Commont tags   */
/*-------------------------*/
#define TIFFTAG_NEWSUBFILETYPE			0xFE	/* subfile data descriptor */
#define TIFFTAG_SUBFILETYPE				0xFF	/* +kind of data in subfile */
#define TIFFTAG_IMAGEWIDTH				0x100	/* image width in pixels */
#define TIFFTAG_IMAGELENGTH				0x101	/* image height in pixels */
#define TIFFTAG_BITSPERSAMPLE			0x102	/* bits per channel (sample) */
#define TIFFTAG_COMPRESSION				0x103	/* data compression technique */
#define TIFFTAG_PHOTOMETRIC				0x106	/* photometric interpretation */
#define TIFFTAG_THRESHHOLDING			0x107	/* +thresholding used on data */
#define TIFFTAG_FILLORDER				0x10A	/* data order within a byte */
#define TIFFTAG_DOCUMENTNAME			0x10D	/* name of doc. image is from */
#define TIFFTAG_IMAGEDESCRIPTION		0x10E	/* info about image */
#define TIFFTAG_MAKE					0x10F	/* scanner manufacturer name */
#define TIFFTAG_MODEL					0x110	/* scanner model name/number */
#define TIFFTAG_STRIPOFFSETS			0x111	/* offsets to data strips */
#define TIFFTAG_ORIENTATION				0x112	/* +image orientation */
#define TIFFTAG_SAMPLESPERPIXEL			0x115	/* samples per pixel */
#define TIFFTAG_STRIPBYTECOUNTS			0x117	/* bytes counts for strips */
#define TIFFTAG_MINSAMPLEVALUE			0x118	/* +minimum sample value */
#define TIFFTAG_MAXSAMPLEVALUE			0x119	/* +maximum sample value */
#define TIFFTAG_XRESOLUTION				0x11A	/* pixels/resolution in x */
#define TIFFTAG_YRESOLUTION				0x11B	/* pixels/resolution in y */
#define TIFFTAG_PLANARCONFIG			0x11C	/* storage organization */
#define TIFFTAG_PAGENAME				0x11D	/* page name image is from */
#define TIFFTAG_XPOSITION				0x11E	/* x page offset of image lhs */
#define TIFFTAG_YPOSITION				0x11F	/* y page offset of image lhs */
#define TIFFTAG_FREEOFFSETS				0x120	/* +byte offset to free block */
#define TIFFTAG_FREEBYTECOUNTS			0x121	/* +sizes of free blocks */
#define TIFFTAG_GRAYRESPONSEUNIT		0x122	/* $gray scale curve accuracy */
#define TIFFTAG_GRAYRESPONSECURVE		0x123	/* $gray scale response curve */
#define TIFFTAG_RESOLUTIONUNIT			0x128	/* units of resolutions */
#define TIFFTAG_PAGENUMBER				0x129	/* page numbers of multi-page */
#define TIFFTAG_COLORRESPONSEUNIT		0x12C	/* $color curve accuracy */
#define TIFFTAG_TRANSFERFUNCTION		0x12D	/* !colorimetry info */
#define TIFFTAG_SOFTWARE				0x131	/* name & release */
#define TIFFTAG_DATETIME				0x132	/* creation date and time */
#define TIFFTAG_ARTIST					0x13B	/* creator of image */
#define TIFFTAG_HOSTCOMPUTER			0x13C	/* machine where created */
#define TIFFTAG_PREDICTOR				0x13D	/* prediction scheme w/ LZW */
#define TIFFTAG_WHITEPOINT				0x13E	/* image white point */
#define TIFFTAG_PRIMARYCHROMATICITIES	0x13F	/* !primary chromaticities */
#define TIFFTAG_COLORMAP				0x140	/* RGB map for palette image */

namespace Common {
class SeekableReadStream;
}

namespace Graphics {
struct Surface;
}

namespace Image {
namespace Ultima {
namespace Shared {
namespace Gfx {

/**
 * @defgroup image_tiff TIFF decoder (FMTowns specific)
 * @ingroup image
 *
 * @brief Decoder for images encoded as Tagged Image File Format (TIFF) FMTowns specific.
 *
 * Used in engines:
 * - Ultima 1 (FMTowns)
 * @{
 */

class TIFFDecoder : public ImageDecoder {
public:

/*-------------------------*/
/* structure of TIFF head  */
/*-------------------------*/
typedef struct {
	short tiff_magic;      /* magic number (defines byte order) */
	short tiff_version;    /* TIFF version number */
	long  tiff_diroff;     /* byte offset to first directory */
} TIFFHeaderClassic;

/*-------------------------*/
/* structure of IFD field  */
/*-------------------------*/
typedef struct {
	short tag;
	short type;
	long  count;
	long  value;
} IFDEntry;

/*----------------------------*/
/* structure of Rational data */
/*----------------------------*/
typedef struct  {
	long  value_c ;
	long  value_m ;
} Rational;

/*-------------------------*/
/* MaxSampleValue          */
/*-------------------------*/
typedef struct {
	short strip1 ;
	short strip2 ;
	short strip3 ;
} MaxValue;

/*------------------------------------------*/
/* Definition of TOWNS standard TIFF header */
/*------------------------------------------*/
typedef struct {
	TIFFHeaderClassic		head           ; /* header                     8 */
	short          			ifdnum         ; /* ifd offset                10 */
	IFDEntry				tags[15]       ; /* tags                     238 */
	char	       			_pad1[18]      ; /* padding                  256 */
	unsigned short 			pal16[3][16]   ; /* 16c palette              352 */
	char           			_pad2[16*9-12] ; /* padding                  484 */
	MaxValue       			MaxSValue      ; /* MaxSampleValue           490 */
	short	       			bpp[3]         ; /* SamplesPerPixel 24bit    496 */
	Rational       			xres,yres      ; /* resolution X,Y           512 */
	unsigned short 			pal256[3][256] ; /* 256c palette            2048 */
} TownsHeader;

	TIFFDecoder();
	virtual ~TIFFDecoder();

	// ImageDecoder API
	void destroy();
	bool loadStream(Common::SeekableReadStream &stream);
	const TownsHeader *getHeader() const { return &_header; }
	const Graphics::Surface *getSurface() const { return _surface; }
	const byte *getPalette() const { return _palette; }
	uint16 getPaletteColorCount() const { return _paletteColorCount; }
private:

	typedef enum {
		TIFFBO_UNKNOWN = 0, TIFFBO_BIGENDIAN = 1, TIFFBO_LITTLEENDIAN = 2,
	} TIFFByteOrder;

	TIFFByteOrder _byte_order;
	TownsHeader _header;
	Graphics::Surface *_surface;
	uint16 _width;
	uint16 _height;
	byte _bits_per_pixel;
	byte _sample_per_pixel;
	byte _photometric_int;
	byte _compression;
	byte _fill_order;
	byte _min_sample;
	byte _max_sample;
	byte *_palette;
	uint16 _offset;
	uint16 _paletteColorCount;
	uint8 _numRelevantPlanes;


	void loadHeader(Common::SeekableReadStream &stream);
	void processImageFileDirectory();
	void setBitsPerPixel(long value);
	void loadPalette(Common::SeekableReadStream &stream, const uint32 size);
	void loadBitmap(Common::SeekableReadStream &stream);
};
/** @} */
} // End of namespace Gfx 
} // End of namespace Shared
} // End of namespace Ultima 
} // End of namespace Image

#endif
