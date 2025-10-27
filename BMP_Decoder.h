#ifndef BMP_DECODER_H
#define BMP_DECODER_H

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#define SUCCESSFUL_DECODING 0
#define ERROR_FILE_OVER_1_GIGABYTE -1
#define ERROR_OVER_1_BILLION_PIXELS -2
#define ERROR_UNRECOGNIZED_BIT_DEPTH -4
#define ERROR_2_BIT_IMAGES_ARE_NOT_SUPPORTED -5
#define ERROR_48_BIT_IMAGES_ARE_NOT_SUPPORTED -6
#define ERROR_64_BIT_IMAGES_ARE_NOT_SUPPORTED -7
#define ERROR_COMPRESSED_1_BIT_IMAGES_ARE_NOT_SUPPORTED -8
#define ERROR_COMPRESSSED_24_BIT_IMAGES_ARE_NOT_SUPPORTED -9
#define ERROR_COLOR_PROFILES_ARE_NOT_SUPPORTED -10
#define ERROR_HALFTONING_IS_NOT_SUPPORTED -11

enum class BMP_Scanline_Order
{
    TOP_DOWN,
    BOTTOM_UP
};

enum class BMP_Compression_Method
{
    BI_RGB_ = 0,
    BI_RLE8_ = 1,
    BI_RLE4_ = 2,
    BI_BITFIELDS_ = 3,
    BI_JPEG_ = 4,
    BI_PNG_ = 5,
    BI_ALPHABITFIELDS_ = 6,
    BI_CMYK_ = 11,
    BI_CMYKRLE8_ = 12,
    BI_CMYKRLE4_ = 13
};

enum class BMP_Halftoning_Algorithm
{
    NONE = 0,
    ERROR_DIFFUSION = 1,
    PANDA = 2,
    SUPER_CIRCLE = 3
};

struct BMP_RGBA
{
    unsigned char R;
    unsigned char G;
    unsigned char B;
    unsigned char A;
};

//UINT_MAX value of an unsigned int field means that the field is not set
struct BMP_Image
{
    int Width;
    int Height;
    int BitDepth;
    BMP_Compression_Method CompressionMethod;
    unsigned int PixelsPerMeterX;
    unsigned int PixelsPerMeterY;
    unsigned int UsedColorsCount;
    unsigned int ImportantColorsCount;
    enum BMP_Scanline_Order ScanlineOrder;
    unsigned int RedMask;
    unsigned int GreenMask;
    unsigned int BlueMask;
    unsigned int AlphaMask;
    unsigned int ColorSpaceType;
    unsigned int CIE_X_Red;
    unsigned int CIE_Y_Red;
    unsigned int CIE_Z_Red;
    unsigned int CIE_X_Green;
    unsigned int CIE_Y_Green;
    unsigned int CIE_Z_Green;
    unsigned int CIE_X_Blue;
    unsigned int CIE_Y_Blue;
    unsigned int CIE_Z_Blue;
    unsigned int GammaRed;
    unsigned int GammaGreen;
    unsigned int GammaBlue;
    unsigned int Intent;
    unsigned int ProfileData;
    unsigned int ProfileSize;
    BMP_Halftoning_Algorithm HalftoningAlgorithm;
    unsigned int HalftoningParameter1;
    unsigned int HalftoningParameter2;
    BMP_RGBA* ColorTable = nullptr;
    int NumberOfColors; //number of colors in ColorTable
    BMP_RGBA* Pixels = nullptr;
    int NumberOfPixels;
};

//(MUTATE _image)
//if _generatePixelData is false, then only metadata is generated (no pixel data)
/* if _outputScanlineOrder is TOP_DOWN then the first line in the generated image is the topmost line of the input image and if
   it's BOTTOM_UP then the first line in the generated image is the bottom-most line of the input image */
//the generated image is 32bpp
/* parameter _file represents the contents of a .BMP file (it's size is maximum 1GB), (it contains maximum 1 billion pixels)
   and the contents are (structurally and semantically valid) (no validation is performed at all) -> */
int BMP_Decode(unsigned char* _file, bool _generatePixelData, BMP_Image* _image, BMP_Scanline_Order _outputScanlineOrder);

//(MUTATE _image)
//if _generatePixelData is false, then only metadata is generated (no pixel data)
/* if _outputScanlineOrder is TOP_DOWN then the first line in the generated image is the topmost line of the input image and if
   it's BOTTOM_UP then the first line in the generated image is the bottom-most line of the input image */
//the generated image is 32bpp
/* the file specified by _filepath (exists), (it's size is maximum 1GB), (it contains maximum 1 billion pixels)
   and is (structurally and semantically valid) (no validation is performed at all) -> */
int BMP_Decode(const wchar_t* _filepath, bool _generatePixelData, BMP_Image* _image, BMP_Scanline_Order _outputScanlineOrder);

#endif
