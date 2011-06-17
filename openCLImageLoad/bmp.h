#ifndef __BMP_H
#define __BMP_H

#include <stdint.h>

#pragma pack(2)
struct bmp_header {
	uint16_t magic;
	uint32_t file_size;
	uint16_t reserved0;
	uint16_t reserved1;
	uint32_t offset;
};

#define BMP_MAGIC	0x424D

struct dib_header {
	uint32_t header_size;
	int32_t width;		/* Pixels */
	int32_t height;	/* Pixels */
	uint16_t color_planes;
	uint16_t bpp;
	uint32_t compression;
	uint32_t image_size;
	int32_t horizontal;	/* Pixels per meter */
	int32_t vertical;		/* Pixels per meter */
	uint32_t colors;
	uint32_t important_colors;
};
#pragma pack()

#define BI_RGB		0x00
#define BI_RLE8	0x01
#define BI_RLE4	0x02
#define BI_BITFIELDS	0x03
#define BI_JPEG	0x04
#define BI_PNG		0x05

#endif
