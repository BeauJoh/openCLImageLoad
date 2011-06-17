//
//  io_tiff.cpp
//  ReadTifIntensity
//
//  Created by Beau Johnston on 28/04/11.
//  Copyright 2011 University Of New England. All rights reserved.
//

#include "io_tiff.h"


uint32 _imageLength, _imageWidth, _imageOrientation, _config, _bitsPerSample, _samplesPerPixel, _photometric, _rowsPerStrip, _bytesPerPixel, _linebytes;
uint64 _tiffScanLineSize;

uint16 * read_tiff(char*fileName){
    
    TIFF * tif = TIFFOpen((char *)fileName,"r");
    
    
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &_imageLength);
    TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &_config);
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &_imageWidth);
    TIFFGetField(tif, TIFFTAG_ORIENTATION, &_imageOrientation);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &_bitsPerSample);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &_samplesPerPixel);
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &_photometric);
    TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &_rowsPerStrip);    
    
    _bytesPerPixel = _bitsPerSample/8;

    if (_imageOrientation == 0) {
        _imageOrientation = ORIENTATION_TOPLEFT;
    }
    
    //size of each row
    _linebytes = _samplesPerPixel * _imageWidth * _bytesPerPixel;
    
    uint16* image = new uint16[_linebytes *_imageLength];

    uint16* rowbuff = NULL;
    
    rowbuff = (uint16*) _TIFFmalloc(_samplesPerPixel * _bytesPerPixel * _imageWidth); 
    for(int row=0;row <_imageLength;row++)
    {
        if (TIFFReadScanline(tif, (void*)rowbuff, row, 0) < 0){
            printf("oh no! Something bad happened at reading the image %s to file\n", fileName);
            break;  
        }
        
            //else we get a valid row buffer, and we process it to the return image
        for(int col=0; col<_imageWidth;col++){
            image[(row*_linebytes)+col] = rowbuff[col];
        }
    }
    _TIFFfree(rowbuff);
    
    //close the file handle and done! :)
    TIFFClose(tif);
    
    return image;
}


void write_tiff(char* fileName, uint16 * image){
    
    //open tif file handle
    TIFF * tif = TIFFOpen(fileName, "w");
    
    //if the file opened successfully
    if(tif){
        
        // local variables
        uint16* rowbuff = NULL;
        
        // set tag file format tags in the header
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, _imageLength);
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, _imageWidth);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, _config);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, _bitsPerSample);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, _samplesPerPixel);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, _imageOrientation);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, _photometric);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, _rowsPerStrip);
        
        
        // allocate memory for the buffer for writing, 
        //      hint:   it has to be big enough for a strip (1 row at a time)
        //              as we process row by row
        rowbuff = (uint16*) _TIFFmalloc(_linebytes); 
        
        for (uint32 row = 0; row < _imageLength; row++){
            // Use string.h's ability to copy memory locations directly,
            // this little baby copies entire row of image array and puts 
            // it into the buffer for writing
            
            for(int col=0; col<_imageWidth;col++){
                    rowbuff[col] = image[(row*_linebytes) + col];
            }
            
            //then write the row, out to file
            //if it failed notify the user of something bad
            if (TIFFWriteScanline(tif, rowbuff, row,0) < 0){
                printf("oh no! Something bad happened at writing the image %s to file\n", fileName);
                break;
            }
        }
        
        //release the buffer
        _TIFFfree(rowbuff);
        
        //close the file handle and done! :)
        TIFFClose(tif);
        
        delete image;
        return;
    }
    
    printf("Whops... The image %s dosn't exit to write", fileName);
    return;

}

unsigned char * read_tiff_rgb_8_bit_from_file(char* fileName)
{
    
    // open tif file handle
    TIFF* tif = TIFFOpen(fileName, "r");
    
    // if the file opened successfully
    if (tif) {
        // local variables
        tdata_t buf;
        uint32 row;
        
        // collect tag file format tags from the header
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &_imageLength);
        TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &_config);
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &_imageWidth);
        TIFFGetField(tif, TIFFTAG_ORIENTATION, &_imageOrientation);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &_bitsPerSample);
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &_samplesPerPixel);
        TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &_photometric);
        TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &_rowsPerStrip);
        
        // if image orientation hasn't been set up 
        // (as some images do not conform to tiff protocol)
        // libtiff will return zero, so we give it a proper value
        // i.e. set the image in the top left
        if (_imageOrientation == 0) {
            _imageOrientation = ORIENTATION_TOPLEFT;
        }
        
        // size of each row
        tsize_t linebytes = _samplesPerPixel * _imageWidth;
        
        printf("_imageLength = %i\n", _imageLength);
        printf("_config = %i\n", _config);
        printf("_imageWidth = %i\n", _imageWidth);
        printf("_imageOrientation = %i\n", _imageOrientation);
        printf("_bitsPerSample = %i\n", _bitsPerSample);
        printf("_samplesPerPixel = %i\n", _samplesPerPixel);
        printf("_photometric = %i\n", _photometric);
        printf("_rowsPerStrip = %i\n", _rowsPerStrip);
        
        
        // allocate memory for the buffer for reading, 
        //      hint:   it has to be big enough for a strip (1 row at a time)
        //              as we process row by row
        buf = _TIFFmalloc(linebytes);
        
        
        // create the correct amount of memory for the image 
        unsigned char image [_imageWidth*_imageLength*_samplesPerPixel];
        
        // if its a grayscale image
        if (_config == PLANARCONFIG_CONTIG) {
            // process each row _imageLength can also be though of as the height 
            // or (y axis) of the image
            uint16 s;

            for (s = 0; s < _samplesPerPixel; s++){

            for (row = 0; row < _imageLength; row++){
                
                //read each row
                TIFFReadScanline(tif, buf, row, s);
                
                // Use string.h's ability to copy memory locations directly,
                // this little baby copies the entire row of the buffer and puts 
                // it into the newly malloc'd image array at specified row
                memcpy(&image[row*_imageWidth], buf, linebytes);
            }
            }
            
            // we arn't dealing with a grayscale (Intensity) image
        } else if (_config == PLANARCONFIG_SEPARATE) {
            
            // private variables for more than 1 channel image i.e. 
            // rgb/rgba/ra basically anything but Intensity (grayscale)
            uint16 s;
            // we need to iterate though each component of each pixel
            // i.e. if we had rgb. We will traverse though all the red, then green, then blue.
            for (s = 0; s < _samplesPerPixel; s++){
                // traverse each row
                for (row = 0; row < _imageLength; row++){
                    // read that line of components (r,g,b or alpha ect)
                    TIFFReadScanline(tif, buf, row, s);
                    // store that buffer row into the image array
                    memcpy(&image[row*_imageWidth], buf, linebytes);
                }
            }
        }
        
        //release the buffer
        _TIFFfree(buf);
        buf = NULL;
        
        //close the file handle and done! :)
        TIFFClose(tif);
        
        //create a pointer to the image and return it
        unsigned char * ptr = image;
        return ptr;
    }
    
    printf("Whops... The image %s dosn't exit to read", fileName);
    unsigned char * ptr = NULL;
    return ptr;
}

void write_tiff_rgb_8_bit_from_file(char* fileName, unsigned char * image){
    
    //open tif file handle
    TIFF * tif = TIFFOpen(fileName, "w");
    
    //if the file opened successfully
    if(tif){
        
        // local variables
        tdata_t buf;
        uint32 row;
        
        //size of each row
        tsize_t linebytes = _samplesPerPixel * _imageWidth;
        
        // set tag file format tags in the header
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, _imageLength);
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, _imageWidth);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, _config);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, _bitsPerSample);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, _samplesPerPixel);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, _imageOrientation);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, _photometric);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, _rowsPerStrip);
        
        
        // allocate memory for the buffer for writing, 
        //      hint:   it has to be big enough for a strip (1 row at a time)
        //              as we process row by row
        buf = _TIFFmalloc(linebytes);
        
        uint16 s;

        for (s = 0; s < _samplesPerPixel; s++){

            for (row = 0; row < _imageLength; row++){
                    // Use string.h's ability to copy memory locations directly,
                    // this little baby copies entire row of image array and puts 
                    // it into the buffer for writing
                memcpy(buf, &image[row *_imageWidth], linebytes);
            
                    //then write the row, out to file
                    //if it failed notify the user of something bad
                if (TIFFWriteScanline(tif, buf, row, s) < 0){
                    printf("oh no! Something bad happened at writing the image %s to file\n", fileName);
                    break;
                }
            }
        }
        
        //release the buffer
        _TIFFfree(buf);
        buf = NULL;
        
        //close the file handle and done! :)
        TIFFClose(tif);
        return;
    }
    
    printf("Whops... The image %s dosn't exit to write", fileName);
    return;
}



uint16 * read_tiff_strip_file(char* fileName)
{
    
        // open tif file handle
    TIFF* tif = TIFFOpen(fileName, "r");

        // if the file opened successfully
    if (tif) {
            // local variables
        tdata_t buf;
        uint32 row;
        
            // collect tag file format tags from the header
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &_imageLength);
        TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &_config);
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &_imageWidth);
        TIFFGetField(tif, TIFFTAG_ORIENTATION, &_imageOrientation);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &_bitsPerSample);
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &_samplesPerPixel);
        TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &_photometric);
        TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &_rowsPerStrip);

            // if image orientation hasn't been set up 
            // (as some images do not conform to tiff protocol)
            // libtiff will return zero, so we give it a proper value
            // i.e. set the image in the top left
        if (_imageOrientation == 0) {
            _imageOrientation = ORIENTATION_TOPLEFT;
        }
        uint32 _bytesPerPixel = _bitsPerSample/8;

        
            // size of each row
        tsize_t linebytes = _samplesPerPixel * _imageWidth * _bytesPerPixel;
        
        
            // allocate memory for the buffer for reading, 
            //      hint:   it has to be big enough for a strip (1 row at a time)
            //              as we process row by row
        buf = _TIFFmalloc(linebytes);
        
            // create the correct amount of memory for the image 
        uint16 image [_imageLength*linebytes];
        
            // if its a grayscale image
        if (_config == PLANARCONFIG_CONTIG) {
                // process each row _imageLength can also be though of as the height 
                // or (y axis) of the image
            for (row = 0; row < _imageLength; row++){
    
                    //read each row
                TIFFReadScanline(tif, buf, row);
                
                    // Use string.h's ability to copy memory locations directly,
                    // this little baby copies the entire row of the buffer and puts 
                    // it into the newly malloc'd image array at specified row
                memcpy(&image[row*_imageWidth], buf, linebytes);
            }
            
            // we arn't dealing with a grayscale (Intensity) image
        } else if (_config == PLANARCONFIG_SEPARATE) {
            
                // private variables for more than 1 channel image i.e. 
                // rgb/rgba/ra basically anything but Intensity (grayscale)
            uint16 s;
                // we need to iterate though each component of each pixel
                // i.e. if we had rgb. We will traverse though all the red, then green, then blue.
            for (s = 0; s < _samplesPerPixel; s++){
                    // traverse each row
                for (row = 0; row < _imageLength; row++){
                            // read that line of components (r,g,b or alpha ect)
                        TIFFReadScanline(tif, buf, row, s);
                            // store that buffer row into the image array
                        memcpy(&image[(row+s)*_imageWidth], buf, linebytes);
                }
            }
        }
        
        printf("Samples per pixel = %i\n", _samplesPerPixel);
        printf("Bytes per pixel = %i\n", _bytesPerPixel);



            //release the buffer
        _TIFFfree(buf);
        buf = NULL;
        
            //close the file handle and done! :)
        TIFFClose(tif);
        
            //create a pointer to the image and return it
        uint16 * ptr = image;
        return ptr;
    }
    
    printf("Whops... The image %s dosn't exit to read", fileName);
    uint16 * ptr = NULL;
    return ptr;
}


void write_tiff_strip_file(char* fileName, uint16 * image){
    
        //open tif file handle
    TIFF * tif = TIFFOpen(fileName, "w");
    
        //if the file opened successfully
    if(tif){
        
            // local variables
        tdata_t buf;
        uint32 row;
        
            //size of each row
        tsize_t linebytes = _samplesPerPixel * _imageWidth;
        
            // set tag file format tags in the header
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, _imageLength);
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, _imageWidth);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, _config);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, _bitsPerSample);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, _samplesPerPixel);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, _imageOrientation);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, _photometric);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, _rowsPerStrip);

        
            // allocate memory for the buffer for writing, 
            //      hint:   it has to be big enough for a strip (1 row at a time)
            //              as we process row by row
        buf = _TIFFmalloc(linebytes);

        for (row = 0; row < _imageLength; row++){
                // Use string.h's ability to copy memory locations directly,
                // this little baby copies entire row of image array and puts 
                // it into the buffer for writing
            memcpy(buf, &image[row *_imageWidth], linebytes);

                //then write the row, out to file
                //if it failed notify the user of something bad
            if (TIFFWriteScanline(tif, buf, row, 0) < 0){
                printf("oh no! Something bad happened at writing the image %s to file\n", fileName);
                break;
            }
        }
        
            //release the buffer
        _TIFFfree(buf);
        buf = NULL;
        
            //close the file handle and done! :)
        TIFFClose(tif);
        return;
    }
    
    printf("Whops... The image %s dosn't exit to write", fileName);
    return;
}

uint32 getImageLength(void){
    return _imageLength;
};

uint32 getImageWidth(void){
    return _imageWidth;
};

uint32 getImageOrientation(void){
    return _imageOrientation;
};

uint32 getConfig(void){
    return _config;
};

uint32 getBitsPerSample(void){
    return _bitsPerSample;
};

uint32 getSamplesPerPixel(void){
    return _samplesPerPixel;
};

uint32 getPhotometric(void){
    return _photometric;
};

uint32 getRowsPerStrip(void){
    return _rowsPerStrip;
};

uint32 getImageRowPitch(void){
    return _samplesPerPixel * _imageWidth;
};

uint32 getImageSlicePitch(void){
    return _rowsPerStrip;
};

//Inline Nomalizes Data In Image
uint16 * normalizeData(uint16* image){

    uint16 * returnImage = new uint16[_linebytes *_imageLength];
    
    for (uint32 i=0; i< _imageLength*_linebytes; i++) {
        returnImage[i] = (image[i]/(2^_bitsPerSample));
        
#ifdef DEBUG
        if (returnImage[i] != 0) {
            printf("new normalized image is  %hu from %hu @ %d\n", returnImage[i], image[i], i);
        }
#endif
    }

    delete image;
    
    return returnImage;
};


uint16 * unNormalizeData(uint16* image){
    uint16 * returnImage = new uint16[_linebytes *_imageLength];

    for (uint32 i=0; i<_imageLength*_linebytes; i++) {
        returnImage[i] = image[i]*(2^_bitsPerSample);
        
#ifdef DEBUG
        if (returnImage[i] != 0) {
            printf("new unnormalized image is  %hu from %hu @ %d\n", returnImage[i], image[i], i);
        }
#endif
    }
    
    delete image;
    
    return returnImage;
};
