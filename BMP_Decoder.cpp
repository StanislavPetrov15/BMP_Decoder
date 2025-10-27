#include "BMP_Decoder.h"

//(PRIVATE)
unsigned char ReadI8(const unsigned char* _array, int& _position)
{
    unsigned char x = _array[_position];
    _position++;
    return x;
}

//(PRIVATE)
unsigned short ReadI16(const unsigned char* _array, int& _position)
{
    unsigned char byte1 = _array[_position];
    unsigned char byte2 = _array[_position + 1];
    _position += 2;
     return byte1 | (byte2 << 8);
}

//(PRIVATE)
unsigned int ReadI32(const unsigned char* _array, int& _position)
{
    unsigned char byte1 = _array[_position];
    unsigned char byte2 = _array[_position + 1];
    unsigned char byte3 = _array[_position + 2];
    unsigned char byte4 = _array[_position + 3];
    _position += 4;
    return (byte4 << 24) | (byte3 << 16) | (byte2 << 8) | byte1;
}

//(PRIVATE)
//_index >= 0 || _index <= 7 ->
bool GetBit(unsigned char _number, int _index)
{
    _number = _number << (7 - _index);
    _number = _number >> 7;
    return _number;
}

//(PRIVATE)
//GetBits(53, 1, 3) => [0, 1, 0] = Dx2
//_begin >= 0 || _end <= 7, _begin < _end ->
unsigned char GetBits(unsigned char _number, int _begin, int _end)
{
    _number = _number << (7 - _end);
    _number = _number >> ((7 - _end) + _begin);
    return _number;
}

//(PRIVATE)
int NumberOfLeadingZeros(unsigned int _number)
{
    if (_number == 0) return 8;

    int count = 0;

    while (true)
    {
        unsigned int number = _number;
        number = number << 31;
        number = number >> 31;

        if (number == 1)
        {
            return count;
        }
        else
        {
            _number >>= 1;
            count++;
        }
    }
}

//(PRIVATE)
int Abs(int _number)
{
    if (_number >= 0)
    {
        return _number;
    }
    else
    {
        return _number - _number - _number;
    }
}

int Mod(int, int);

//(PRIVATE)
bool IsEven(int N)
{
    return Mod(Abs(N), 2) == 0;
}

//(PRIVATE)
bool IsOdd(int N)
{
    return Mod(Abs(N), 2) != 0;
}

//(PRIVATE)
int Mod(int N1, int N2)
{
    if (N1 < 0 || N2 < 0)
    {
        return 0;
    }
    else if (N1 == N2)
    {
        return 0;
    }
    else if (N1 < N2)
    {
        return N1;
    }
    else
    {
        return N1 - ((N1 / N2) * N2);
    }
}

//(MUTATE _image)
//if _generatePixelData is false, then only metadata is generated (no pixel data)
/* if _outputScanlineOrder is TOP_DOWN then the first line in the generated image is the topmost line of the input image and if
   it's BOTTOM_UP then the first line in the generated image is the bottom-most line of the input image */
//the generated image is 32bpp
/* parameter _file represents the contents of a .BMP file (it's size is maximum 1GB), (it contains maximum 1 billion pixels)
   and the contents are (structurally and semantically valid) (no validation is performed at all) -> */
int BMP_Decode(unsigned char* _file, bool _generatePixelData, BMP_Image* _image, BMP_Scanline_Order _outputScanlineOrder)
{
    BMP_Image* image = _image;

    static const int BITMAP_CORE_HEADER_SIZE = 12;
    static const int OS22X_BITMAP_HEADER_SHORT_SIZE = 16;
    static const int OS22X_BITMAP_HEADER_LONG_SIZE = 64;
    static const int BITMAP_INFO_HEADER_V1_SIZE = 40;
    static const int BITMAP_INFO_HEADER_V2_SIZE = 52;
    static const int BITMAP_INFO_HEADER_V3_SIZE = 56;
    static const int BITMAP_INFO_HEADER_V4_SIZE = 108;
    static const int BITMAP_INFO_HEADER_V5_SIZE = 124;

    int statusCode = 0; //non-terminating error can occur and the error must be recorded

    //READING THE FILE HEADER

    int filePosition = 10;

    int pixelArrayPosition = ReadI32(_file, filePosition);

    //(STATE) fileBufferPosition = 14

    //READING THE DIB-HEADER

    int headerSize = ReadI32(_file, filePosition);
    int bitmapSize;

    if (headerSize == BITMAP_CORE_HEADER_SIZE)
    {
        image->Width = ReadI16(_file, filePosition);
        image->Height = ReadI16(_file, filePosition);
        filePosition += 2; //ignoring the field NumberOfColorPlanes
        image->BitDepth = ReadI16(_file, filePosition);
        image->CompressionMethod = BMP_Compression_Method::BI_RGB_;
    }
    else
    {
        //BITMAP_INFO_HEADER_V1

        image->Width = ReadI32(_file, filePosition);
        image->Height = ReadI32(_file, filePosition);
        filePosition += 2; //ignoring the field NumberOfColorPlanes
        image->BitDepth = ReadI16(_file, filePosition);
        image->CompressionMethod = static_cast<BMP_Compression_Method>(ReadI32(_file, filePosition));
        bitmapSize = ReadI32(_file, filePosition);
        image->PixelsPerMeterX = ReadI32(_file, filePosition);
        image->PixelsPerMeterY = ReadI32(_file, filePosition);
        image->UsedColorsCount = ReadI32(_file, filePosition);
        image->ImportantColorsCount = ReadI32(_file, filePosition);

        //(->)

        //(STATE) the type of the DIB-header is OS22X_BITMAP_HEADER_SHORT | OS22X_BITMAP_HEADER_LONG | BITMAP_INFO_HEADER_V2..v5

       if (headerSize == OS22X_BITMAP_HEADER_SHORT_SIZE)
       {
            filePosition += 2; //ignoring the field Units
            filePosition += 2; //ignoring alignment field
            filePosition += 2; //ignoring the field RecordingAlgorithm
            image->HalftoningAlgorithm = static_cast<BMP_Halftoning_Algorithm>(ReadI16(_file, filePosition));
            image->HalftoningParameter1 = ReadI32(_file, filePosition);
            image->HalftoningParameter2 = ReadI32(_file, filePosition);
            filePosition += 4; //ignoring the field ColorEncoding
            filePosition += 4; //ignoring reserved field
       }
       else if (headerSize == OS22X_BITMAP_HEADER_LONG_SIZE)
       {
           filePosition += 2; //ignoring the field Units
           filePosition += 2; //ignoring alignment field
           filePosition += 2; //ignoring the field RecordingAlgorithm
           image->HalftoningAlgorithm = static_cast<BMP_Halftoning_Algorithm>(ReadI16(_file, filePosition));
           image->HalftoningParameter1 = ReadI32(_file, filePosition);
           image->HalftoningParameter2 = ReadI32(_file, filePosition);
       }
       else
       {
           //BITMAP_INFO_HEADER_V2
           if (headerSize > BITMAP_INFO_HEADER_V1_SIZE || (image->CompressionMethod == BMP_Compression_Method::BI_BITFIELDS_))
           {
               image->RedMask = ReadI32(_file, filePosition);
               image->GreenMask = ReadI32(_file, filePosition);
               image->BlueMask = ReadI32(_file, filePosition);
           }

           //(->)

           //BITMAP_INFO_HEADER_V3
           if (headerSize > BITMAP_INFO_HEADER_V1_SIZE || (image->CompressionMethod == BMP_Compression_Method::BI_ALPHABITFIELDS_))
           {
               image->AlphaMask = ReadI32(_file, filePosition);
           }

           //(->)

            //BITMAP_INFO_HEADER_V4
            if (headerSize > BITMAP_INFO_HEADER_V3_SIZE)
            {
                image->ColorSpaceType = ReadI32(_file, filePosition);
                image->CIE_X_Red = ReadI32(_file, filePosition);
                image->CIE_Y_Red = ReadI32(_file, filePosition);
                image->CIE_Z_Red = ReadI32(_file, filePosition);
                image->CIE_X_Green = ReadI32(_file, filePosition);
                image->CIE_Y_Green = ReadI32(_file, filePosition);
                image->CIE_Z_Green = ReadI32(_file, filePosition);
                image->CIE_X_Blue = ReadI32(_file, filePosition);
                image->CIE_Y_Blue = ReadI32(_file, filePosition);
                image->CIE_Z_Blue = ReadI32(_file, filePosition);
                image->GammaRed = ReadI32(_file, filePosition);
                image->GammaGreen = ReadI32(_file, filePosition);
                image->GammaBlue = ReadI32(_file, filePosition);
            }

            //(->)

            //BITMAP_INFO_HEADER_V5
            if (headerSize > BITMAP_INFO_HEADER_V4_SIZE)
            {
                image->Intent = ReadI32(_file, filePosition);
                image->ProfileData = ReadI32(_file, filePosition);
                image->ProfileSize = ReadI32(_file, filePosition);
                filePosition += 4; //ignoring reserved field
            }
       }
    }

    if (image->Width * image->Height > 1000000000)
    {
        return ERROR_OVER_1_BILLION_PIXELS;
    }

    if (image->BitDepth == 2)
    {
         return ERROR_2_BIT_IMAGES_ARE_NOT_SUPPORTED;
    }
    else if (image->BitDepth == 48)
    {
        return ERROR_48_BIT_IMAGES_ARE_NOT_SUPPORTED;
    }
    else if (image->BitDepth == 64)
    {
        return ERROR_64_BIT_IMAGES_ARE_NOT_SUPPORTED;
    }
    else if (image->BitDepth == 1 && image->CompressionMethod != BMP_Compression_Method::BI_RGB_)
    {
        return ERROR_COMPRESSED_1_BIT_IMAGES_ARE_NOT_SUPPORTED;
    }
    else if (image->BitDepth == 24 &&
             image->CompressionMethod != BMP_Compression_Method::BI_RGB_ &&
             image->CompressionMethod != BMP_Compression_Method::BI_BITFIELDS_ &&
             image->CompressionMethod != BMP_Compression_Method::BI_ALPHABITFIELDS_)
    {
        return ERROR_COMPRESSSED_24_BIT_IMAGES_ARE_NOT_SUPPORTED;
    }
    else if (headerSize == BITMAP_INFO_HEADER_V5_SIZE)
    {
        statusCode = ERROR_COLOR_PROFILES_ARE_NOT_SUPPORTED;
    }
    else if (headerSize == OS22X_BITMAP_HEADER_SHORT_SIZE || headerSize == OS22X_BITMAP_HEADER_LONG_SIZE)
    {
        statusCode = ERROR_HALFTONING_IS_NOT_SUPPORTED;
    }

    //(->)

    //reading the color table if the image is 1, 4 or 8bpp
    if (image->BitDepth <= 8)
    {
         int paletteLength;

         if (image->BitDepth == 1)
         {
             paletteLength = 2;
         }
         else if (image->BitDepth == 4)
         {
             paletteLength = 16;
         }
         else if (image->BitDepth == 8)
         {
             paletteLength = 256;
         }

        //if image->ColorTable is not set by external code
        if (image->ColorTable == nullptr)
        {
           image->ColorTable = new BMP_RGBA[paletteLength];
        }

         int colorIndex = 0;

         for (int i = 0; i < paletteLength; i++)
         {
             BMP_RGBA color;
             color.B = ReadI8(_file, filePosition);
             color.G = ReadI8(_file, filePosition);
             color.R = ReadI8(_file, filePosition);
             color.A = 255;
             image->ColorTable[colorIndex++] = color;

             if (headerSize != BITMAP_CORE_HEADER_SIZE)
             {
                 filePosition++; //ignoring the alpha field
             }
         }
    }

    if (!_generatePixelData)
    {
        return statusCode;
    }

    filePosition = pixelArrayPosition;

    if (image->Height < 0)
    {
        image->ScanlineOrder = BMP_Scanline_Order::TOP_DOWN;
        image->Height = Abs(image->Height);
    }
    else
    {
        image->ScanlineOrder = BMP_Scanline_Order::BOTTOM_UP;
    }

    //if image->Pixels is not set by external code
    if (image->Pixels == nullptr)
    {
        image->Pixels = new BMP_RGBA[image->Width * image->Height];
    }

    //READING THE PIXEL ARRAY

    if (image->BitDepth == 1)
    {
        int meaningfulRowBytes = image->Width < 8 ? 1 : (image->Width / 8);

        if (image->Width % 8 != 0) meaningfulRowBytes++;

        int rowPadding = meaningfulRowBytes;

        while (rowPadding % 4 != 0) rowPadding++;

        rowPadding -= meaningfulRowBytes;

        //for every line

        int rowIndex = image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN ? 0 : image->Height - 1;

        while (true)
        {
            //for every meaningful byte (8 pixels) in the line
            for (int rowByteIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex++)
            {
                unsigned char rowByte = ReadI8(_file, filePosition);

                //for every bit(pixel) in the byte; bits are read in reverse order, because the most significant bit specifies the the leftmost pixel
                for (int bitIndex = 7; bitIndex > -1; bitIndex--)
                {
                    int columnIndex = (rowByteIndex * 8) + (7 - bitIndex);

                    if (columnIndex == image->Width)
                    {
                        break; //go to next line
                    }

                    if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                    {
                        image->Pixels[(rowIndex * image->Width) + columnIndex] = image->ColorTable[GetBit(rowByte, bitIndex)];
                    }
                    else
                    {
                        image->Pixels[(((image->Height - rowIndex) - 1) * image->Width) + columnIndex] = image->ColorTable[GetBit(rowByte, bitIndex)];
                    }
                }
            }

            //ignoring the padding at the end of the line
            filePosition += rowPadding;

            if (image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN && rowIndex++ == image->Height - 1)
            {
                break;
            }
            else if (image->ScanlineOrder == BMP_Scanline_Order::BOTTOM_UP && rowIndex-- == 0)
            {
                break;
            }
        }
    }
    else if (image->BitDepth == 4 && image->CompressionMethod == BMP_Compression_Method::BI_RGB_)
    {
        int meaningfulRowBytes = image->Width / 2;

        if (image->Width % 4 != 0) meaningfulRowBytes++;

        int rowPadding = meaningfulRowBytes;

        while (rowPadding % 4 != 0) rowPadding++;

        rowPadding -= meaningfulRowBytes;

        //for every line

        int rowIndex = image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN ? 0 : image->Height - 1;

        while (true)
        {
            //for every meaningful byte (2 pixels) in the line
            for (int rowByteIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex++)
            {
                unsigned char rowByte = ReadI8(_file, filePosition);

                //the pixel specified by the most significant nibble (i.e. the left pixel)

                int columnIndex = rowByteIndex * 2;

                if (columnIndex == image->Width)
                {
                    break; //go to next line
                }

                if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                {
                    image->Pixels[rowIndex * image->Width + columnIndex] = image->ColorTable[GetBits(rowByte, 4, 7)];
                }
                else
                {
                    image->Pixels[((image->Height - rowIndex) - 1) * image->Width + columnIndex] = image->ColorTable[GetBits(rowByte, 4, 7)];
                }

                //the pixel specified by the least significant nibble (i.e. the right pixel)

                columnIndex = (rowByteIndex * 2) + 1;

                if (columnIndex == image->Width)
                {
                    break; //go to next line
                }

                if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                {
                    image->Pixels[rowIndex * image->Width + columnIndex] = image->ColorTable[GetBits(rowByte, 0, 3)];
                }
                else
                {
                    image->Pixels[((image->Height - rowIndex) - 1) * image->Width + columnIndex] = image->ColorTable[GetBits(rowByte, 0, 3)];
                }
            }

            //ignoring the padding at the end of the line
            filePosition += rowPadding;

            if (image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN && rowIndex++ == image->Height - 1)
            {
                break;
            }
            else if (image->ScanlineOrder == BMP_Scanline_Order::BOTTOM_UP && rowIndex-- == 0)
            {
                break;
            }
        }
    }
    else if (image->BitDepth == 4 && image->CompressionMethod == BMP_Compression_Method::BI_RLE4_)
    {
        int rowIndex = image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN ? 0 : image->Height - 1;
        int columnIndex = 0;

        //for every byte pair (encoded or absolute)
        while (true)
        {
                unsigned char byte1 = ReadI8(_file, filePosition);
                unsigned char byte2 = ReadI8(_file, filePosition);

               //if the pair is encoded
               if (byte1 > 0)
               {
                    unsigned char firstColorIndex = GetBits(byte2, 4, 7);
                    unsigned char secondColorIndex = GetBits(byte2, 0, 3);

                    //for every pixel in this run-length sequence
                    for (int i = 0; i < byte1; i++)
                    {
                        if (IsEven(i))
                        {
                            if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                            {
                                image->Pixels[rowIndex * image->Width + columnIndex] = image->ColorTable[firstColorIndex];
                            }
                            else
                            {
                                image->Pixels[((image->Height - rowIndex) - 1) * image->Width + columnIndex] = image->ColorTable[firstColorIndex];
                            }
                        }
                        else
                        {
                            if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                            {
                                image->Pixels[rowIndex * image->Width + columnIndex] = image->ColorTable[secondColorIndex];
                            }
                            else
                            {
                                image->Pixels[((image->Height - rowIndex) - 1) * image->Width + columnIndex] = image->ColorTable[firstColorIndex];
                            }
                        }

                        columnIndex++;
                    }
               }
               //if the pair specifies an escape sequence
               else if (byte1 == 0 && byte2 <= 2)
               {
                    //'end of line' sequence
                    if (byte2 == 0)
                    {
                        columnIndex = 0;

                        if (image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN && rowIndex++ == image->Height - 1)
                        {
                            rowIndex++;
                        }
                        else if (image->ScanlineOrder == BMP_Scanline_Order::BOTTOM_UP)
                        {
                            rowIndex--;
                        }
                    }
                    //'end of bitmap' sequence
                    else if (byte2 == 1)
                    {
                        break;
                    }
                    //'delta' sequence ((!) not tested)
                    else
                    {
                        columnIndex += ReadI8(_file, filePosition);
                        rowIndex += ReadI8(_file, filePosition);
                    }
               }
               //(STATE) the pair specifies 'absolute' (i.e. non-compressed) sequence
               else
               {
                    unsigned char numberOfPixels = byte2;
                    int numberOfBytesToRead = numberOfPixels % 2 == 0 ? numberOfPixels / 2 : (numberOfPixels / 2) + 1;
                    int pixelCounter_ = 0;

                    if (IsOdd(numberOfBytesToRead))
                    {
                        numberOfBytesToRead++;
                    }

                    for (int i = 0; i < numberOfBytesToRead; i++)
                    {
                        unsigned char byte = ReadI8(_file, filePosition);

                        //write the first pixel (most significant nibble) if the most significant nibble represents a pixel (i.e. it's not an alignment nibble)
                        if (pixelCounter_ < numberOfPixels)
                        {
                            unsigned char firstColorIndex = GetBits(byte, 4, 7);

                            if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                            {
                                image->Pixels[rowIndex * image->Width + columnIndex] = image->ColorTable[firstColorIndex];
                            }
                            else
                            {
                                image->Pixels[((image->Height - rowIndex) - 1) * image->Width + columnIndex] = image->ColorTable[firstColorIndex];
                            }

                            columnIndex++;
                            pixelCounter_++;
                        }

                        //write the second pixel (least significant nibble) if the least significant nibble represents a pixel (i.e. it's not an alignment nibble)
                        if (pixelCounter_ < numberOfPixels)
                        {
                            unsigned char secondColorIndex = GetBits(byte, 0, 3);

                            if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                            {
                                image->Pixels[rowIndex * image->Width + columnIndex] = image->ColorTable[secondColorIndex];
                            }
                            else
                            {
                                image->Pixels[((image->Height - rowIndex) - 1) * image->Width + columnIndex] = image->ColorTable[secondColorIndex];
                            }

                            columnIndex++;
                            pixelCounter_++;
                        }
                    }
               }
        }
    }
    else if (image->BitDepth == 8 && image->CompressionMethod == BMP_Compression_Method::BI_RGB_)
    {
        int meaningfulRowBytes = image->Width;

        if (meaningfulRowBytes == 0) meaningfulRowBytes++;

        int rowPadding = meaningfulRowBytes;

        while (rowPadding % 4 != 0) rowPadding++;

        rowPadding -= meaningfulRowBytes;

        //for every line

        int rowIndex = image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN ? 0 : image->Height - 1;

        while (true)
        {
            //for every meaningful byte (pixel) in the line
            for (int rowByteIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex++)
            {
                if (rowByteIndex == image->Width)
                {
                    break; //go to next line
                }

                unsigned char rowByte = ReadI8(_file, filePosition);

                if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                {
                    image->Pixels[rowIndex * image->Width + rowByteIndex] = image->ColorTable[rowByte];
                }
                else
                {
                    image->Pixels[((image->Height - rowIndex) - 1) * image->Width + rowByteIndex] = image->ColorTable[rowByte];
                }
            }

            //ignoring the padding-a at the end of the line
            filePosition += rowPadding;

            if (image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN && rowIndex++ == image->Height - 1)
            {
                break;
            }
            else if (image->ScanlineOrder == BMP_Scanline_Order::BOTTOM_UP && rowIndex-- == 0)
            {
                break;
            }
        }
    }
    else if (image->BitDepth == 8 && image->CompressionMethod == BMP_Compression_Method::BI_RLE8_)
    {
        int rowIndex = image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN ? 0 : image->Height - 1;
        int columnIndex = 0;

        //for every byte pair (encoded or absolute)
        while (true)
        {
            unsigned char byte1 = ReadI8(_file, filePosition);
            unsigned char byte2 = ReadI8(_file, filePosition);

            //if the pair is encoded
            if (byte1 > 0)
            {
                //for every pixel in this run-length sequence
                for (int i = 0; i < byte1; i++)
                {
                    if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                    {
                        image->Pixels[rowIndex * image->Width + columnIndex] = image->ColorTable[byte2];
                    }
                    else
                    {
                        image->Pixels[((image->Height - rowIndex) - 1) * image->Width + columnIndex] = image->ColorTable[byte2];
                    }

                    columnIndex++;
                }
            }
            //if the pair specifies an escape sequence
            else if (byte1 == 0 && byte2 <= 2)
            {
                //'end of line' sequence
                if (byte2 == 0)
                {
                    columnIndex = 0;

                    if (image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN && rowIndex++ == image->Height - 1)
                    {
                        rowIndex++;
                    }
                    else if (image->ScanlineOrder == BMP_Scanline_Order::BOTTOM_UP)
                    {
                        rowIndex--;
                    }
                }
                //'end of bitmap' sequence
                else if (byte2 == 1)
                {
                    break;
                }
                //'delta' sequence ((!) it's not tested)
                else
                {
                    columnIndex += ReadI8(_file, filePosition);
                    rowIndex += ReadI8(_file, filePosition);
                }
            }
            //(STATE) the pair represents an 'absolute' (i.e. non-compressed) sequence
            else
            {
                unsigned char numberOfPixels = byte2;
                int numberOfBytesToRead = numberOfPixels % 2 == 0 ? numberOfPixels : numberOfPixels + 1;
                int pixelCounter_ = 0;

                for (int i = 0; i < numberOfBytesToRead; i++)
                {
                    unsigned char byte = ReadI8(_file, filePosition);

                    //write the pixel if this byte represents a pixel (i.e. it's nto an alignment byte)
                    if (pixelCounter_ < numberOfPixels)
                    {
                        unsigned char colorIndex = byte;

                        if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                        {
                            image->Pixels[rowIndex * image->Width + columnIndex] = image->ColorTable[colorIndex];
                        }
                        else
                        {
                            image->Pixels[((image->Height - rowIndex) - 1) * image->Width + columnIndex] = image->ColorTable[colorIndex];
                        }

                        columnIndex++;
                        pixelCounter_++;
                    }
                }
            }
        }
    }
    else if (image->BitDepth == 16)
    {
        int meaningfulRowBytes = image->Width * 2;

        if (meaningfulRowBytes == 0) meaningfulRowBytes++;

        int rowPadding = meaningfulRowBytes;

        while (rowPadding % 4 != 0) rowPadding++;

        rowPadding -= meaningfulRowBytes;

        if (image->CompressionMethod == BMP_Compression_Method::BI_RGB_)
        {
            //for every line

            int rowIndex = image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN ? 0 : image->Height - 1;

            while (true)
            {
                //for every two meaningful byte (pixel) in the line
                for (int rowByteIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex += 2)
                {
                    unsigned short rowWord = ReadI16(_file, filePosition);

                    BMP_RGBA color;

                    color.R = ((rowWord & 0b111110000000000) >> 10) * 8;
                    color.G = ((rowWord & 0b1111100000) >> 5) * 8;
                    color.B = (rowWord & 0b11111) * 8;
                    color.A = 0;

                    int columnIndex = rowByteIndex / 2;

                    if (columnIndex == image->Width)
                    {
                        break; //go to next line
                    }

                    if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                    {
                         image->Pixels[(rowIndex * image->Width) + (rowByteIndex / 2)] = color;
                    }
                    else
                    {
                        image->Pixels[(((image->Height - rowIndex) - 1) * image->Width) + (rowByteIndex / 2)] = color;
                    }
                }

                //ignoring the padding at the end of the line
                filePosition += rowPadding;

                if (image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN && rowIndex++ == image->Height - 1)
                {
                    break;
                }
                else if (image->ScanlineOrder == BMP_Scanline_Order::BOTTOM_UP && rowIndex-- == 0)
                {
                    break;
                }
            }
        }
        else if (image->CompressionMethod == BMP_Compression_Method::BI_BITFIELDS_ || image->CompressionMethod == BMP_Compression_Method::BI_ALPHABITFIELDS_)
        {
            int redMaskLeadingZerosCount = NumberOfLeadingZeros(image->RedMask);
            int greenMaskLeadingZerosCount = NumberOfLeadingZeros(static_cast<unsigned short>(image->GreenMask));
            int blueMaskLeadingZerosCount = NumberOfLeadingZeros(image->BlueMask);
            int alphaMaskLeadingZerosCount = NumberOfLeadingZeros(image->AlphaMask);
            int redComponentMultiplier;
            int greenComponentMultiplier;
            int blueComponentMultiplier;
            int alphaComponentMultiplier;

            //RGB-4444
            if (image->GreenMask == 0b11110000)
            {
                redComponentMultiplier = 16;
                greenComponentMultiplier = 16;
                blueComponentMultiplier = 16;
                alphaComponentMultiplier = 16;
            }
            //RGB-5550
            else if (image->GreenMask == 0b1111100000 && image->CompressionMethod == BMP_Compression_Method::BI_BITFIELDS_)
            {
                redComponentMultiplier = 8;
                greenComponentMultiplier = 8;
                blueComponentMultiplier = 8;
                alphaComponentMultiplier = 0;
            }
            //RGB-5551
            else if (image->GreenMask == 0b1111100000 && image->CompressionMethod == BMP_Compression_Method::BI_ALPHABITFIELDS_)
            {
                redComponentMultiplier = 8;
                greenComponentMultiplier = 8;
                blueComponentMultiplier = 8;
                alphaComponentMultiplier = 255;
            }
            //RGB-565
            else
            {
                redComponentMultiplier = 8;
                greenComponentMultiplier = 4;
                blueComponentMultiplier = 8;
                alphaComponentMultiplier = 0;
            }

            //for every line

            int rowIndex = image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN ? 0 : image->Height - 1;

            while (true)
            {
                //for every two meaningful bytes (pixel) in the line
                for (int rowByteIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex += 2)
                {
                    unsigned short rowWord = ReadI16(_file, filePosition);

                    BMP_RGBA color;

                    unsigned short r = rowWord & image->RedMask;
                    r >>= redMaskLeadingZerosCount;
                    color.R = r * redComponentMultiplier;

                    unsigned short g = rowWord & image->GreenMask;
                    g >>= greenMaskLeadingZerosCount;
                    color.G = g * greenComponentMultiplier;

                    unsigned short b = rowWord & image->BlueMask;
                    b >>= blueMaskLeadingZerosCount;
                    color.B = b * blueComponentMultiplier;

                    if (image->CompressionMethod == BMP_Compression_Method::BI_ALPHABITFIELDS_)
                    {
                        unsigned short a = rowWord & image->AlphaMask;
                        a >>= alphaMaskLeadingZerosCount;
                        color.A = a * alphaComponentMultiplier;
                    }
                    else
                    {
                        color.A = 0;
                    }

                    int columnIndex = rowByteIndex / 2;

                    if (columnIndex == image->Width)
                    {
                        break; //go to next line
                    }

                    if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                    {
                        image->Pixels[(rowIndex * image->Width) + (rowByteIndex / 2)] = color;
                    }
                    else
                    {
                        image->Pixels[(((image->Height - rowIndex) - 1) * image->Width) + (rowByteIndex / 2)] = color;
                    }
                }

                //ignoring the padding at the end of the line
                filePosition += rowPadding;

                if (image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN && rowIndex++ == image->Height - 1)
                {
                    break;
                }
                else if (image->ScanlineOrder == BMP_Scanline_Order::BOTTOM_UP && rowIndex-- == 0)
                {
                    break;
                }
            }
        }
    }
    else if (image->BitDepth == 24)
    {
        int meaningfulRowBytes = image->Width * 3;

        if (meaningfulRowBytes == 0) meaningfulRowBytes++;

        int rowPadding = meaningfulRowBytes;

        while (rowPadding % 4 != 0) rowPadding++;

        rowPadding -= meaningfulRowBytes;

        //for every line

        int rowIndex = image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN ? 0 : image->Height - 1;

        while (true)
        {
            //for every three meaningful bytes (pixel) in the line
            for (int rowByteIndex = 0, columnIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex += 3, columnIndex++)
            {
                BMP_RGBA color;
                color.B = ReadI8(_file, filePosition);
                color.G = ReadI8(_file, filePosition);
                color.R = ReadI8(_file, filePosition);
                color.A = 255;

                if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                {
                    image->Pixels[(rowIndex * image->Width) + columnIndex] = color;
                }
                else
                {
                    image->Pixels[(((image->Height - rowIndex) - 1) * image->Width) + columnIndex] = color;
                }
            }

            //ignoring the padding at the end of the line
            filePosition += rowPadding;

            if (image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN && rowIndex++ == image->Height - 1)
            {
                break;
            }
            else if (image->ScanlineOrder == BMP_Scanline_Order::BOTTOM_UP && rowIndex-- == 0)
            {
                break;
            }
        }
    }
    else if (image->BitDepth == 32)
    {
        if (image->CompressionMethod == BMP_Compression_Method::BI_RGB_)
        {
            //for every line

            int rowIndex = image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN ? 0 : image->Height - 1;

            while (true)
            {
                //for every four meaningful bytes (pixel) in the line
                for (int rowWordIndex = 0; rowWordIndex < image->Width; rowWordIndex++)
                {
                    unsigned int rowWord = ReadI32(_file, filePosition);

                    BMP_RGBA color;

                    color.R = (rowWord & 0b111111110000000000000000) >> 16;
                    color.G = (rowWord & 0b1111111100000000) >> 8;
                    color.B = rowWord & 0b11111111;
                    color.A = 0;
                    //(INSTEAD-OF (PERFORMANCE-REASONS))
                    //color.R = GetBits(rowWord, 16, 23);
                    //color.G = GetBits(rowWord, 8, 15);
                    //color.B = GetBits(rowWord, 0, 7);

                    if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                    {
                        image->Pixels[(rowIndex * image->Width) + rowWordIndex] = color;
                    }
                    else
                    {
                        image->Pixels[(((image->Height - rowIndex) - 1) * image->Width) + rowWordIndex] = color;
                    }
                }

                if (image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN && rowIndex++ == image->Height - 1)
                {
                    break;
                }
                else if (image->ScanlineOrder == BMP_Scanline_Order::BOTTOM_UP && rowIndex-- == 0)
                {
                    break;
                }
            }
        }
        else if (image->CompressionMethod == BMP_Compression_Method::BI_BITFIELDS_ || image->CompressionMethod == BMP_Compression_Method::BI_ALPHABITFIELDS_)
        {
            int redMaskLeadingZerosCount = NumberOfLeadingZeros(image->RedMask);
            int greenMaskLeadingZerosCount = NumberOfLeadingZeros(static_cast<unsigned short>(image->GreenMask));
            int blueMaskLeadingZerosCount = NumberOfLeadingZeros(image->BlueMask);
            int alphaMaskLeadingZerosCount = NumberOfLeadingZeros(image->AlphaMask);
            int componentDivisor = image->RedMask != 61503 ? 1/*RGB-8888*/ : 4/*RGB-1010102*/;
            int alphaComponentMultiplier = image->RedMask != 61503 ? 1/*RGB-8888*/ : 64/*RGB-1010102*/;

            //for every line

            int rowIndex = image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN ? 0 : image->Height - 1;

            while (true)
            {
                //for every four meaningful bytes (pixel) in the line
                for (int rowWordIndex = 0; rowWordIndex < image->Width; rowWordIndex++)
                {
                    unsigned int rowWord = ReadI32(_file, filePosition);

                    BMP_RGBA color;

                    unsigned int r = rowWord & image->RedMask;
                    r >>= redMaskLeadingZerosCount;
                    color.R = r / componentDivisor;

                    unsigned int g = rowWord & image->GreenMask;
                    g >>= greenMaskLeadingZerosCount;
                    color.G = g / componentDivisor;

                    unsigned int b = rowWord & image->BlueMask;
                    b >>= blueMaskLeadingZerosCount;
                    color.B = b / componentDivisor;

                    if (image->CompressionMethod == BMP_Compression_Method::BI_ALPHABITFIELDS_)
                    {
                        unsigned int a = rowWord & image->AlphaMask;
                        a >>= alphaMaskLeadingZerosCount;
                        color.A = a * alphaComponentMultiplier;
                    }
                    else
                    {
                        color.A = 0;
                    }

                    if (_outputScanlineOrder == BMP_Scanline_Order::TOP_DOWN)
                    {
                        image->Pixels[(rowIndex * image->Width) + rowWordIndex] = color;
                    }
                    else
                    {
                        image->Pixels[(((image->Height - rowIndex) - 1) * image->Width) + rowWordIndex] = color;
                    }
                }

                if (image->ScanlineOrder == BMP_Scanline_Order::TOP_DOWN && rowIndex++ == image->Height - 1)
                {
                    break;
                }
                else if (image->ScanlineOrder == BMP_Scanline_Order::BOTTOM_UP && rowIndex-- == 0)
                {
                    break;
                }
            }
        }
    }

    return statusCode;
}

//(MUTATE _image)
//if _generatePixelData is false, then only metadata is generated (no pixel data)
/* if _outputScanlineOrder is TOP_DOWN then the first line in the generated image is the topmost line of the input image and if
   it's BOTTOM_UP then the first line in the generated image is the bottom-most line of the input image */
//the generated image is 32bpp
/* the file specified by _filepath (exists), (it's size is maximum 1GB), (it contains maximum 1 billion pixels)
   and is (structurally and semantically valid) (no validation is performed at all) -> */
int BMP_Decode(const wchar_t* _filepath, bool _generatePixelData, BMP_Image* _image, BMP_Scanline_Order _outputScanlineOrder)
{
    FILE* file = _wfopen(_filepath, L"r+b");

    fseek(file, 0L, SEEK_END);
    int filesize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    if (filesize > 1024 * 1024 * 1024)
    {
        return ERROR_FILE_OVER_1_GIGABYTE;
    }

    unsigned char* fileContents = new unsigned char[filesize];
    fread(fileContents, sizeof(unsigned char), filesize, file);

    fclose(file);

    int result = BMP_Decode(fileContents, _generatePixelData, _image, _outputScanlineOrder);

    delete [] fileContents;

    return result;
}

