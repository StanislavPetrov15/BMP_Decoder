
namespace BMP_Decoder
{
    enum class ScanlineOrder
    {
        TOP_DOWN,
        BOTTOM_UP
    };

    enum class CompressionMethod
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

    enum class HalftoningAlgorithm
    {
        NONE = 0,
        ERROR_DIFFUSION = 1,
        PANDA = 2,
        SUPER_CIRCLE = 3
    };

    struct RGBA
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
         CompressionMethod CompressionMethod;
         int PixelsPerMeterX = UINT_MAX;
         int PixelsPerMeterY = UINT_MAX;
         unsigned int UsedColorsCount = UINT_MAX;
         unsigned int ImportantColorsCount = UINT_MAX;
         enum ScanlineOrder ScanlineOrder;
         unsigned int RedMask = UINT_MAX;
         unsigned int GreenMask = UINT_MAX;
         unsigned int BlueMask = UINT_MAX;
         unsigned int AlphaMask = UINT_MAX;
         unsigned int ColorSpaceType = UINT_MAX;
         unsigned int CIE_X_Red = UINT_MAX;
         unsigned int CIE_Y_Red = UINT_MAX;
         unsigned int CIE_Z_Red = UINT_MAX;
         unsigned int CIE_X_Green = UINT_MAX;
         unsigned int CIE_Y_Green = UINT_MAX;
         unsigned int CIE_Z_Green = UINT_MAX;
         unsigned int CIE_X_Blue = UINT_MAX;
         unsigned int CIE_Y_Blue = UINT_MAX;
         unsigned int CIE_Z_Blue = UINT_MAX;
         unsigned int GammaRed = UINT_MAX;
         unsigned int GammaGreen = UINT_MAX;
         unsigned int GammaBlue = UINT_MAX;
         unsigned int Intent = UINT_MAX;
         unsigned int ProfileData = UINT_MAX;
         unsigned int ProfileSize = UINT_MAX;
         HalftoningAlgorithm HalftoningAlgorithm;
         unsigned int HalftoningParameter1 = UINT_MAX;
         unsigned int HalftoningParameter2 = UINT_MAX;
         list<RGBA> ColorTable;
         list<RGBA> Pixels;
     };

    const int LIBRARY_MAJOR_VERSION = 0;
    const int LIBRARY_MINOR_VERSION = 8;

    const int SUCCESSFUL_DECODING = 0;
    const int ERROR_FILE_OVER_1_GIGABYTE = -1;
    const int ERROR_OVER_1_BILLION_PIXELS = -2;
    const int ERROR_UNRECOGNIZED_BIT_DEPTH = -4;
    const int ERROR_2_BIT_IMAGES_ARE_NOT_SUPPORTED = -10;
    const int ERROR_48_BIT_IMAGES_ARE_NOT_SUPPORTED = -11;
    const int ERROR_64_BIT_IMAGES_ARE_NOT_SUPPORTED = -12;
    const int ERROR_COMPRESSED_1_BIT_IMAGES_ARE_NOT_SUPPORTED = -13;
    const int ERROR_COMPRESSSED_24_BIT_IMAGES_ARE_NOT_SUPPORTED = -14;
    const int ERROR_COLOR_PROFILES_ARE_NOT_SUPPORTED = -15;
    const int ERROR_HALFTONING_IS_NOT_SUPPORTED = -16;

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
        char byte1 = _array[_position];
        char byte2 = _array[_position + 1];

        _position += 2;

        return _16(byte1, byte2, Endianity::LE);
    }

    //(PRIVATE)
    unsigned int ReadI32(const unsigned char* _array, int& _position)
    {
        char byte1 = _array[_position];
        char byte2 = _array[_position + 1];
        char byte3 = _array[_position + 2];
        char byte4 = _array[_position + 3];

        _position += 4;

        return _32(byte1, byte2, byte3, byte4, Endianity::LE);
    }

    //(MUTATE _image)
    //if _generatePixelData is false, then only metadata is generated (no pixel data)
    //the lines int the generated image Top-Bottom order (the first line in the generated array is the topmost line of the image);
    //the generated image is 32bpp
    /* the file specified by _filepath (exists), (it's size is maximum 1GB), (it contains maximum 1 billion pixels)
       and is (structurally and semantically valid) (no validation is performer at all) -> */
    int Decode(const string& _filepath, bool _generatePixelData, BMP_Image& _image)
    {
        int statusCode = 0; //non-terminating error can occur and the error must be recorded

        BMP_Image& image = _image;

        static const int BITMAP_CORE_HEADER_SIZE = 12;
        static const int OS22X_BITMAP_HEADER_SHORT_SIZE = 16;
        static const int OS22X_BITMAP_HEADER_LONG_SIZE = 64;
        static const int BITMAP_INFO_HEADER_V1_SIZE = 40;
        static const int BITMAP_INFO_HEADER_V2_SIZE = 52;
        static const int BITMAP_INFO_HEADER_V3_SIZE = 56;
        static const int BITMAP_INFO_HEADER_V4_SIZE = 108;
        static const int BITMAP_INFO_HEADER_V5_SIZE = 124;

        filesystem::File file(_filepath);

        if (file.size() > 1024 * 1024 * 1024)
        {
            return ERROR_FILE_OVER_1_GIGABYTE;
        }

        unsigned char* fileBuffer = new unsigned char[file.size()];
        file.ReadBlock(fileBuffer, file.size(), false);

        list<char> imageData(file.size(), false);

        file.Close();

        //READING THE FILE HEADER

        int fileBufferPosition = 10;

        int pixelArrayPosition = ReadI32(fileBuffer, fileBufferPosition);

        //(STATE) fileBufferPosition = 14

        //READING THE DIB-HEADER

        int headerSize = ReadI32(fileBuffer, fileBufferPosition);
        int bitmapSize;

        if (headerSize == BITMAP_CORE_HEADER_SIZE)
        {
            image.Width = ReadI16(fileBuffer, fileBufferPosition);
            image.Height = ReadI16(fileBuffer, fileBufferPosition);
            fileBufferPosition += 2; //ignoring the field NumberOfColorPlanes
            image.BitDepth = ReadI16(fileBuffer, fileBufferPosition);
            image.CompressionMethod = CompressionMethod::BI_RGB_;
        }
        else
        {
            //BITMAP_INFO_HEADER_V1

            image.Width = ReadI32(fileBuffer, fileBufferPosition);
            image.Height = ReadI32(fileBuffer, fileBufferPosition);
            fileBufferPosition += 2; //ignoring the field NumberOfColorPlanes
            image.BitDepth = ReadI16(fileBuffer, fileBufferPosition);
            image.CompressionMethod = static_cast<CompressionMethod>(ReadI32(fileBuffer, fileBufferPosition));
            bitmapSize = ReadI32(fileBuffer, fileBufferPosition);
            image.PixelsPerMeterX = ReadI32(fileBuffer, fileBufferPosition);
            image.PixelsPerMeterY = ReadI32(fileBuffer, fileBufferPosition);
            image.UsedColorsCount = ReadI32(fileBuffer, fileBufferPosition);
            image.ImportantColorsCount = ReadI32(fileBuffer, fileBufferPosition);

            //(->)

            //(STATE) the type of the DIB-header is OS22X_BITMAP_HEADER_SHORT | OS22X_BITMAP_HEADER_LONG | BITMAP_INFO_HEADER_V2..v5

           if (headerSize == OS22X_BITMAP_HEADER_SHORT_SIZE)
           {
                fileBufferPosition += 2; //ignoring the field Units
                fileBufferPosition += 2; //ignoring alignment field
                fileBufferPosition += 2; //ignoring the field RecordingAlgorithm
                image.HalftoningAlgorithm = static_cast<HalftoningAlgorithm>(ReadI16(fileBuffer, fileBufferPosition));
                image.HalftoningParameter1 = ReadI32(fileBuffer, fileBufferPosition);
                image.HalftoningParameter2 = ReadI32(fileBuffer, fileBufferPosition);
                fileBufferPosition += 4; //ignoring the field ColorEncoding
                fileBufferPosition += 4; //ignoring reserved field
           }
           else if (headerSize == OS22X_BITMAP_HEADER_LONG_SIZE)
           {
               fileBufferPosition += 2; //ignoring the field Units
               fileBufferPosition += 2; //ignoring alignment field
               fileBufferPosition += 2; //ignoring the field RecordingAlgorithm
               image.HalftoningAlgorithm = static_cast<HalftoningAlgorithm>(ReadI16(fileBuffer, fileBufferPosition));
               image.HalftoningParameter1 = ReadI32(fileBuffer, fileBufferPosition);
               image.HalftoningParameter2 = ReadI32(fileBuffer, fileBufferPosition);
           }
           else
           {
               //BITMAP_INFO_HEADER_V2
               if (headerSize > BITMAP_INFO_HEADER_V1_SIZE || (image.CompressionMethod == CompressionMethod::BI_BITFIELDS_))
               {
                   image.RedMask = ReadI32(fileBuffer, fileBufferPosition);
                   image.GreenMask = ReadI32(fileBuffer, fileBufferPosition);
                   image.BlueMask = ReadI32(fileBuffer, fileBufferPosition);
               }

               //(->)

               //BITMAP_INFO_HEADER_V3
               if (headerSize > BITMAP_INFO_HEADER_V1_SIZE || (image.CompressionMethod == CompressionMethod::BI_ALPHABITFIELDS_))
               {
                   image.AlphaMask = ReadI32(fileBuffer, fileBufferPosition);
               }

               //(->)

                //BITMAP_INFO_HEADER_V4
                if (headerSize > BITMAP_INFO_HEADER_V3_SIZE)
                {
                    image.ColorSpaceType = ReadI32(fileBuffer, fileBufferPosition);
                    image.CIE_X_Red = ReadI32(fileBuffer, fileBufferPosition);
                    image.CIE_Y_Red = ReadI32(fileBuffer, fileBufferPosition);
                    image.CIE_Z_Red = ReadI32(fileBuffer, fileBufferPosition);
                    image.CIE_X_Green = ReadI32(fileBuffer, fileBufferPosition);
                    image.CIE_Y_Green = ReadI32(fileBuffer, fileBufferPosition);
                    image.CIE_Z_Green = ReadI32(fileBuffer, fileBufferPosition);
                    image.CIE_X_Blue = ReadI32(fileBuffer, fileBufferPosition);
                    image.CIE_Y_Blue = ReadI32(fileBuffer, fileBufferPosition);
                    image.CIE_Z_Blue = ReadI32(fileBuffer, fileBufferPosition);
                    image.GammaRed = ReadI32(fileBuffer, fileBufferPosition);
                    image.GammaGreen = ReadI32(fileBuffer, fileBufferPosition);
                    image.GammaBlue = ReadI32(fileBuffer, fileBufferPosition);
                }

                //(->)

                //BITMAP_INFO_HEADER_V5
                if (headerSize > BITMAP_INFO_HEADER_V4_SIZE)
                {
                    image.Intent = ReadI32(fileBuffer, fileBufferPosition);
                    image.ProfileData = ReadI32(fileBuffer, fileBufferPosition);
                    image.ProfileSize = ReadI32(fileBuffer, fileBufferPosition);
                    fileBufferPosition += 4; //ignoring reserved field
                }
           }
        }

        if (image.Width * image.Height > 1'000'000'000)
        {
            return ERROR_OVER_1_BILLION_PIXELS;
        }

        if (image.BitDepth == 2)
        {
             delete [] fileBuffer;
             return ERROR_2_BIT_IMAGES_ARE_NOT_SUPPORTED;
        }
        else if (image.BitDepth == 48)
        {
            delete [] fileBuffer;
            return ERROR_48_BIT_IMAGES_ARE_NOT_SUPPORTED;
        }
        else if (image.BitDepth == 64)
        {
            delete [] fileBuffer;
            return ERROR_64_BIT_IMAGES_ARE_NOT_SUPPORTED;
        }
        else if (image.BitDepth == 1 && image.CompressionMethod != CompressionMethod::BI_RGB_)
        {
            delete [] fileBuffer;
            return ERROR_COMPRESSED_1_BIT_IMAGES_ARE_NOT_SUPPORTED;
        }
        else if (image.BitDepth == 24 &&
                 image.CompressionMethod != CompressionMethod::BI_RGB_ &&
                 image.CompressionMethod != CompressionMethod::BI_BITFIELDS_ &&
                 image.CompressionMethod != CompressionMethod::BI_ALPHABITFIELDS_)
        {
            delete [] fileBuffer;
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
        if (image.BitDepth <= 8)
        {
             int paletteLength;

             if (image.BitDepth == 1)
             {
                 paletteLength = 2;
             }
             else if (image.BitDepth == 4)
             {
                 paletteLength = 16;
             }
             else if (image.BitDepth == 8)
             {
                 paletteLength = 256;
             }

             for (int i = 0; i < paletteLength; i++)
             {
                 RGBA color;
                 color.B = ReadI8(fileBuffer, fileBufferPosition);
                 color.G = ReadI8(fileBuffer, fileBufferPosition);
                 color.R = ReadI8(fileBuffer, fileBufferPosition);
                 color.A = 255;
                 image.ColorTable.Append(color);

                 if (headerSize != BITMAP_CORE_HEADER_SIZE)
                 {
                     fileBufferPosition++; //ignoring the alpha field
                 }
             }
        }

        if (!_generatePixelData)
        {
            delete [] fileBuffer;
            return statusCode;
        }

        fileBufferPosition = pixelArrayPosition;

        if (image.Height < 0)
        {
            image.ScanlineOrder = ScanlineOrder::TOP_DOWN;
            image.Height = numeric::Absolute(image.Height);
        }
        else
        {
            image.ScanlineOrder = ScanlineOrder::BOTTOM_UP;
        }

        image.Pixels = list<RGBA>(image.Width * image.Height, true);

        //READING THE PIXEL ARRAY
        if (image.BitDepth == 1)
        {
            int meaningfulRowBytes = image.Width < 8 ? 1 : (image.Width / 8);

            if (image.Width % 8 != 0) meaningfulRowBytes++;

            int rowPadding = meaningfulRowBytes;

            while (rowPadding % 4 != 0) rowPadding++;

            rowPadding -= meaningfulRowBytes;

            //for every line

            int rowIndex = image.ScanlineOrder == ScanlineOrder::TOP_DOWN ? 0 : image.Height - 1;

            while (true)
            {
                //for every meaningful byte (8 pixels) in the line
                for (int rowByteIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex++)
                {
                    unsigned char rowByte = ReadI8(fileBuffer, fileBufferPosition);

                    //for every bit(pixel) in the byte; bits are read in reverse order, because the most significant bit specifies the the leftmost pixel
                    for (int bitIndex = 7; bitIndex > -1; bitIndex--)
                    {
                        int columnIndex = (rowByteIndex * 8) + (7 - bitIndex);

                        if (columnIndex == image.Width)
                        {
                            break; //go to next line
                        }

                        image.Pixels[(rowIndex * image.Width) + columnIndex] = image.ColorTable[bit_operations::GetBit(rowByte, bitIndex)];
                    }
                }

                //ignoring the padding at the end of the line
                fileBufferPosition += rowPadding;

                if (image.ScanlineOrder == ScanlineOrder::TOP_DOWN && rowIndex++ == image.Height - 1)
                {
                    break;
                }
                else if (image.ScanlineOrder == ScanlineOrder::BOTTOM_UP && rowIndex-- == 0)
                {
                    break;
                }
            }
        }
        else if (image.BitDepth == 4 && image.CompressionMethod == CompressionMethod::BI_RGB_)
        {
            int meaningfulRowBytes = image.Width / 2;

            if (image.Width % 4 != 0) meaningfulRowBytes++;

            int rowPadding = meaningfulRowBytes;

            while (rowPadding % 4 != 0) rowPadding++;

            rowPadding -= meaningfulRowBytes;

            //for every line

            int rowIndex = image.ScanlineOrder == ScanlineOrder::TOP_DOWN ? 0 : image.Height - 1;

            while (true)
            {
                //for every meaningful byte (2 pixels) in the line
                for (int rowByteIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex++)
                {
                    unsigned char rowByte = ReadI8(fileBuffer, fileBufferPosition);

                    //the pixel specified by the most significant nibble (i.e. the left pixel)

                    int columnIndex = rowByteIndex * 2;

                    if (columnIndex == image.Width)
                    {
                        break; //go to next line
                    }

                    image.Pixels[rowIndex * image.Width + columnIndex] = image.ColorTable[bit_operations::GetBits(rowByte, 4, 7)];

                    //the pixel specified by the least significant nibble (i.e. the right pixel)

                    columnIndex = (rowByteIndex * 2) + 1;

                    if (columnIndex == image.Width)
                    {
                        break; //go to next line
                    }

                    image.Pixels[rowIndex * image.Width + columnIndex] = image.ColorTable[bit_operations::GetBits(rowByte, 0, 3)];
                }

                //ignoring the padding at the end of the line
                fileBufferPosition += rowPadding;

                if (image.ScanlineOrder == ScanlineOrder::TOP_DOWN && rowIndex++ == image.Height - 1)
                {
                    break;
                }
                else if (image.ScanlineOrder == ScanlineOrder::BOTTOM_UP && rowIndex-- == 0)
                {
                    break;
                }
            }
        }
        else if (image.BitDepth == 4 && image.CompressionMethod == CompressionMethod::BI_RLE4_)
        {
            int rowIndex = image.ScanlineOrder == ScanlineOrder::TOP_DOWN ? 0 : image.Height - 1;
            int columnIndex = 0;

            //for every byte pair (encoded or absolute)
            while (true)
            {
                    unsigned char byte1 = ReadI8(fileBuffer, fileBufferPosition);
                    unsigned char byte2 = ReadI8(fileBuffer, fileBufferPosition);

                   //if the pair is encoded
                   if (byte1 > 0)
                   {
                        unsigned char firstColorIndex = bit_operations::GetBits(byte2, 4, 7);
                        unsigned char secondColorIndex = bit_operations::GetBits(byte2, 0, 3);

                        //for every pixel in this run-length sequence
                        for (int i = 0; i < byte1; i++)
                        {
                            if (numeric::IsEven(i))
                            {
                                image.Pixels[rowIndex * image.Width + columnIndex] = image.ColorTable[firstColorIndex];
                            }
                            else
                            {
                                image.Pixels[rowIndex * image.Width + columnIndex] = image.ColorTable[secondColorIndex];
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

                            if (image.ScanlineOrder == ScanlineOrder::TOP_DOWN && rowIndex++ == image.Height - 1)
                            {
                                rowIndex++;
                            }
                            else if (image.ScanlineOrder == ScanlineOrder::BOTTOM_UP)
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
                            columnIndex += ReadI8(fileBuffer, fileBufferPosition);
                            rowIndex += ReadI8(fileBuffer, fileBufferPosition);
                        }
                   }
                   //(STATE) the pair specifies 'absolute' (i.e. non-compressed) sequence
                   else
                   {
                        unsigned char numberOfPixels = byte2;
                        int numberOfBytesToRead = numberOfPixels % 2 == 0 ? numberOfPixels / 2 : (numberOfPixels / 2) + 1;
                        int pixelCounter_ = 0;

                        if (numeric::IsOdd(numberOfBytesToRead))
                        {
                            numberOfBytesToRead++;
                        }

                        for (int i = 0; i < numberOfBytesToRead; i++)
                        {
                            unsigned char byte = ReadI8(fileBuffer, fileBufferPosition);

                            //write the first pixel (most significant nibble) if the most significant nibble represents a pixel (i.e. it's not an alignment nibble)
                            if (pixelCounter_ < numberOfPixels)
                            {
                                unsigned char firstColorIndex = bit_operations::GetBits(byte, 4, 7);

                                image.Pixels[rowIndex * image.Width + columnIndex] = image.ColorTable[firstColorIndex];

                                columnIndex++;
                                pixelCounter_++;
                            }

                            //write the second pixel (least significant nibble) if the least significant nibble represents a pixel (i.e. it's not an alignment nibble)
                            if (pixelCounter_ < numberOfPixels)
                            {
                                unsigned char secondColorIndex = bit_operations::GetBits(byte, 0, 3);

                                image.Pixels[rowIndex * image.Width + columnIndex] = image.ColorTable[secondColorIndex];

                                columnIndex++;
                                pixelCounter_++;
                            }
                        }
                   }
            }
        }
        else if (image.BitDepth == 8 && image.CompressionMethod == CompressionMethod::BI_RGB_)
        {
            int meaningfulRowBytes = image.Width;

            if (meaningfulRowBytes == 0) meaningfulRowBytes++;

            int rowPadding = meaningfulRowBytes;

            while (rowPadding % 4 != 0) rowPadding++;

            rowPadding -= meaningfulRowBytes;

            //for every line

            int rowIndex = image.ScanlineOrder == ScanlineOrder::TOP_DOWN ? 0 : image.Height - 1;

            while (true)
            {
                //for every meaningful byte (pixel) in the line
                for (int rowByteIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex++)
                {
                    if (rowByteIndex == image.Width)
                    {
                        break; //go to next line
                    }

                    unsigned char rowByte = ReadI8(fileBuffer, fileBufferPosition);
                    image.Pixels[rowIndex * image.Width + rowByteIndex] = image.ColorTable[rowByte];
                }

                //ignoring the padding-a at the end of the line
                fileBufferPosition += rowPadding;

                if (image.ScanlineOrder == ScanlineOrder::TOP_DOWN && rowIndex++ == image.Height - 1)
                {
                    break;
                }
                else if (image.ScanlineOrder == ScanlineOrder::BOTTOM_UP && rowIndex-- == 0)
                {
                    break;
                }
            }
        }
        else if (image.BitDepth == 8 && image.CompressionMethod == CompressionMethod::BI_RLE8_)
        {
            int rowIndex = image.ScanlineOrder == ScanlineOrder::TOP_DOWN ? 0 : image.Height - 1;
            int columnIndex = 0;

            //for every byte pair (encoded or absolute)
            while (true)
            {
                unsigned char byte1 = ReadI8(fileBuffer, fileBufferPosition);
                unsigned char byte2 = ReadI8(fileBuffer, fileBufferPosition);

                //if the pair is encoded
                if (byte1 > 0)
                {
                    //for every pixel in this run-length sequence
                    for (int i = 0; i < byte1; i++)
                    {
                        image.Pixels[rowIndex * image.Width + columnIndex] = image.ColorTable[byte2];

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

                        if (image.ScanlineOrder == ScanlineOrder::TOP_DOWN && rowIndex++ == image.Height - 1)
                        {
                            rowIndex++;
                        }
                        else if (image.ScanlineOrder == ScanlineOrder::BOTTOM_UP)
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
                        columnIndex += ReadI8(fileBuffer, fileBufferPosition);
                        rowIndex += ReadI8(fileBuffer, fileBufferPosition);
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
                        unsigned char byte = ReadI8(fileBuffer, fileBufferPosition);

                        //write the pixel if this byte represents a pixel (i.e. it's nto an alignment byte)
                        if (pixelCounter_ < numberOfPixels)
                        {
                            unsigned char colorIndex = byte;

                            image.Pixels[rowIndex * image.Width + columnIndex] = image.ColorTable[colorIndex];

                            columnIndex++;
                            pixelCounter_++;
                        }
                    }
                }
            }
        }
        else if (image.BitDepth == 16)
        {
            int meaningfulRowBytes = image.Width * 2;

            if (meaningfulRowBytes == 0) meaningfulRowBytes++;

            int rowPadding = meaningfulRowBytes;

            while (rowPadding % 4 != 0) rowPadding++;

            rowPadding -= meaningfulRowBytes;

            if (image.CompressionMethod == CompressionMethod::BI_RGB_)
            {
                //for every line

                int rowIndex = image.ScanlineOrder == ScanlineOrder::TOP_DOWN ? 0 : image.Height - 1;

                while (true)
                {
                    //for every two meaningful byte (pixel) in the line
                    for (int rowByteIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex += 2)
                    {
                        unsigned short rowWord = ReadI16(fileBuffer, fileBufferPosition);

                        RGBA color;

                        color.R = ((rowWord & 0b111110000000000) >> 10) * 8;
                        color.G = ((rowWord & 0b1111100000) >> 5) * 8;
                        color.B = (rowWord & 0b11111) * 8;
                        color.A = 0;

                        int columnIndex = rowByteIndex / 2;

                        if (columnIndex == image.Width)
                        {
                            break; //go to next line
                        }

                        image.Pixels[(rowIndex * image.Width) + (rowByteIndex / 2)] = color;
                    }

                    //ignoring the padding at the end of the line
                    fileBufferPosition += rowPadding;

                    if (image.ScanlineOrder == ScanlineOrder::TOP_DOWN && rowIndex++ == image.Height - 1)
                    {
                        break;
                    }
                    else if (image.ScanlineOrder == ScanlineOrder::BOTTOM_UP && rowIndex-- == 0)
                    {
                        break;
                    }
                }
            }
            else if (image.CompressionMethod == CompressionMethod::BI_BITFIELDS_ || image.CompressionMethod == CompressionMethod::BI_ALPHABITFIELDS_)
            {
                int redMaskLeadingZerosCount = bit_operations::NumberOfLeadingZeros(image.RedMask);
                int greenMaskLeadingZerosCount = bit_operations::NumberOfLeadingZeros(static_cast<unsigned short>(image.GreenMask));
                int blueMaskLeadingZerosCount = bit_operations::NumberOfLeadingZeros(image.BlueMask);
                int alphaMaskLeadingZerosCount = bit_operations::NumberOfLeadingZeros(image.AlphaMask);
                int redComponentMultiplier;
                int greenComponentMultiplier;
                int blueComponentMultiplier;
                int alphaComponentMultiplier;

                //RGB-4444
                if (image.GreenMask == 0b11110000)
                {
                    redComponentMultiplier = 16;
                    greenComponentMultiplier = 16;
                    blueComponentMultiplier = 16;
                    alphaComponentMultiplier = 16;
                }
                //RGB-5550
                else if (image.GreenMask == 0b1111100000 && image.CompressionMethod == CompressionMethod::BI_BITFIELDS_)
                {
                    redComponentMultiplier = 8;
                    greenComponentMultiplier = 8;
                    blueComponentMultiplier = 8;
                    alphaComponentMultiplier = 0;
                }
                //RGB-5551
                else if (image.GreenMask == 0b1111100000 && image.CompressionMethod == CompressionMethod::BI_ALPHABITFIELDS_)
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

                int rowIndex = image.ScanlineOrder == ScanlineOrder::TOP_DOWN ? 0 : image.Height - 1;

                while (true)
                {
                    //for every two meaningful bytes (pixel) in the line
                    for (int rowByteIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex += 2)
                    {
                        unsigned short rowWord = ReadI16(fileBuffer, fileBufferPosition);

                        RGBA color;

                        unsigned short r = rowWord & image.RedMask;
                        r >>= redMaskLeadingZerosCount;
                        color.R = r * redComponentMultiplier;

                        unsigned short g = rowWord & image.GreenMask;
                        g >>= greenMaskLeadingZerosCount;
                        color.G = g * greenComponentMultiplier;

                        unsigned short b = rowWord & image.BlueMask;
                        b >>= blueMaskLeadingZerosCount;
                        color.B = b * blueComponentMultiplier;

                        if (image.CompressionMethod == CompressionMethod::BI_ALPHABITFIELDS_)
                        {
                            unsigned short a = rowWord & image.AlphaMask;
                            a >>= alphaMaskLeadingZerosCount;
                            color.A = a * alphaComponentMultiplier;
                        }
                        else
                        {
                            color.A = 0;
                        }

                        int columnIndex = rowByteIndex / 2;

                        if (columnIndex == image.Width)
                        {
                            break; //go to next line
                        }

                        image.Pixels[(rowIndex * image.Width) + (rowByteIndex / 2)] = color;
                    }

                    //ignoring the padding at the end of the line
                    fileBufferPosition += rowPadding;

                    if (image.ScanlineOrder == ScanlineOrder::TOP_DOWN && rowIndex++ == image.Height - 1)
                    {
                        break;
                    }
                    else if (image.ScanlineOrder == ScanlineOrder::BOTTOM_UP && rowIndex-- == 0)
                    {
                        break;
                    }
                }
            }
        }
        else if (image.BitDepth == 24)
        {
            int meaningfulRowBytes = image.Width * 3;

            if (meaningfulRowBytes == 0) meaningfulRowBytes++;

            int rowPadding = meaningfulRowBytes;

            while (rowPadding % 4 != 0) rowPadding++;

            rowPadding -= meaningfulRowBytes;

            //for every line

            int rowIndex = image.ScanlineOrder == ScanlineOrder::TOP_DOWN ? 0 : image.Height - 1;

            while (true)
            {
                //for every three meaningful bytes (pixel) in the line
                for (int rowByteIndex = 0, columnIndex = 0; rowByteIndex < meaningfulRowBytes; rowByteIndex += 3, columnIndex++)
                {
                    RGBA color;
                    color.B = ReadI8(fileBuffer, fileBufferPosition);
                    color.G = ReadI8(fileBuffer, fileBufferPosition);
                    color.R = ReadI8(fileBuffer, fileBufferPosition);
                    color.A = 255;

                    image.Pixels[(rowIndex * image.Width) + columnIndex] = color;
                }

                //ignoring the padding at the end of the line
                fileBufferPosition += rowPadding;

                if (image.ScanlineOrder == ScanlineOrder::TOP_DOWN && rowIndex++ == image.Height - 1)
                {
                    break;
                }
                else if (image.ScanlineOrder == ScanlineOrder::BOTTOM_UP && rowIndex-- == 0)
                {
                    break;
                }
            }
        }
        else if (image.BitDepth == 32)
        {
            if (image.CompressionMethod == CompressionMethod::BI_RGB_)
            {
                //for every line

                int rowIndex = image.ScanlineOrder == ScanlineOrder::TOP_DOWN ? 0 : image.Height - 1;

                while (true)
                {
                    //for every four meaningful bytes (pixel) in the line
                    for (int rowWordIndex = 0; rowWordIndex < image.Width; rowWordIndex++)
                    {
                        unsigned int rowWord = ReadI32(fileBuffer, fileBufferPosition);

                        RGBA color;

                        color.R = (rowWord & 0b111111110000000000000000) >> 16;
                        color.G = (rowWord & 0b1111111100000000) >> 8;
                        color.B = rowWord & 0b11111111;
                        color.A = 0;
                        //(INSTEAD-OF (PERFORMANCE-REASONS))
                        //color.R = bit_operations::GetBits(rowWord, 16, 23);
                        //color.G = bit_operations::GetBits(rowWord, 8, 15);
                        //color.B = bit_operations::GetBits(rowWord, 0, 7);

                        image.Pixels[(rowIndex * image.Width) + rowWordIndex] = color;
                    }

                    if (image.ScanlineOrder == ScanlineOrder::TOP_DOWN && rowIndex++ == image.Height - 1)
                    {
                        break;
                    }
                    else if (image.ScanlineOrder == ScanlineOrder::BOTTOM_UP && rowIndex-- == 0)
                    {
                        break;
                    }
                }
            }
            else if (image.CompressionMethod == CompressionMethod::BI_BITFIELDS_ || image.CompressionMethod == CompressionMethod::BI_ALPHABITFIELDS_)
            {
                int redMaskLeadingZerosCount = bit_operations::NumberOfLeadingZeros(image.RedMask);
                int greenMaskLeadingZerosCount = bit_operations::NumberOfLeadingZeros(static_cast<unsigned short>(image.GreenMask));
                int blueMaskLeadingZerosCount = bit_operations::NumberOfLeadingZeros(image.BlueMask);
                int alphaMaskLeadingZerosCount = bit_operations::NumberOfLeadingZeros(image.AlphaMask);
                int componentDivisor = image.RedMask != 61503 ? 1/*RGB-8888*/ : 4/*RGB-1010102*/;
                int alphaComponentMultiplier = image.RedMask != 61503 ? 1/*RGB-8888*/ : 64/*RGB-1010102*/;

                //for every line

                int rowIndex = image.ScanlineOrder == ScanlineOrder::TOP_DOWN ? 0 : image.Height - 1;

                while (true)
                {
                    //for every four meaningful bytes (pixel) in the line
                    for (int rowWordIndex = 0; rowWordIndex < image.Width; rowWordIndex++)
                    {
                        unsigned int rowWord = ReadI32(fileBuffer, fileBufferPosition);

                        RGBA color;

                        unsigned int r = rowWord & image.RedMask;
                        r >>= redMaskLeadingZerosCount;
                        color.R = r / componentDivisor;

                        unsigned int g = rowWord & image.GreenMask;
                        g >>= greenMaskLeadingZerosCount;
                        color.G = g / componentDivisor;

                        unsigned int b = rowWord & image.BlueMask;
                        b >>= blueMaskLeadingZerosCount;
                        color.B = b / componentDivisor;

                        if (image.CompressionMethod == CompressionMethod::BI_ALPHABITFIELDS_)
                        {
                            unsigned int a = rowWord & image.AlphaMask;
                            a >>= alphaMaskLeadingZerosCount;
                            color.A = a * alphaComponentMultiplier;
                        }
                        else
                        {
                            color.A = 0;
                        }

                        image.Pixels[(rowIndex * image.Width) + rowWordIndex] = color;
                    }

                    if (image.ScanlineOrder == ScanlineOrder::TOP_DOWN && rowIndex++ == image.Height - 1)
                    {
                        break;
                    }
                    else if (image.ScanlineOrder == ScanlineOrder::BOTTOM_UP && rowIndex-- == 0)
                    {
                        break;
                    }
                }
            }
        }

        delete [] fileBuffer;

        return statusCode;
    }
