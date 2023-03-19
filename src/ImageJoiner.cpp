#include "ImageJoiner.h"

#include <x86intrin.h>

#include <malloc.h>
#include <assert.h>
#include <stdio.h>

namespace db::image {

  typedef __v8sf vector8f;
  typedef __v8su vector8u;

#pragma pack(push, 1)
  struct FileHeader {
    uint16_t type;
    uint32_t size;
    uint32_t reserved;
    uint32_t offset;
  };
#pragma pack(pop)

#pragma pack(push, 1)
  struct BmpHeader {
    uint32_t size;
    int32_t  width;
    int32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t imageSize;
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colors;
    uint32_t colorsImportant;
  };
#pragma pack(pop)

#pragma pack(push, 1)
  struct ColorInfo {
    uint32_t redMask{ 0x00ff0000 };
    uint32_t greenMask{ 0x0000ff00 };
    uint32_t blueMask{ 0x000000ff };
    uint32_t alphaMask{ 0xff000000 };
    uint32_t colorSpaceType{ 0x73524742 };
    uint32_t unused[16];
  };
#pragma pack(pop)

  void CreateBmp(bmp *bmp)
  {
    assert(bmp);
  }

  void CreateBmp(bmp *bmp, unsigned width, unsigned height)
  {
  }

  void DestroyBmp(bmp *bmp)
  {
    assert(bmp);

    free(bmp->pixels);

    *bmp = {};
  }

  bool Load(bmp *bmp, const char *path)
  {
      FILE *file = fopen(path, "rb");
    assert(file);

    FileHeader fileHeader{};
    BmpHeader   bmpHeader{};
    ColorInfo   colorInfo{};

    fseek(file, 0, SEEK_SET);
    fread(&fileHeader, 1, sizeof(FileHeader), file);

    if (fileHeader.type != 0x4D42) goto fail;

    fread(&bmpHeader, 1, sizeof(BmpHeader), file);

    if (bmpHeader.width  <= 0) goto fail;
    if (bmpHeader.height <= 0) goto fail;

    if (bmpHeader.planes != 1) goto fail;
    if (bmpHeader.bitCount != 32) goto fail;

    if (bmpHeader.compression != 0 &&
        bmpHeader.compression != 3) goto fail;

    fread(&colorInfo, 1, sizeof(ColorInfo), file);

    bmp->pixels = (uint32_t *)
      calloc((size_t) (bmpHeader.width*bmpHeader.height), sizeof(uint32_t));
    assert(bmp->pixels);

    bmp-> width = bmpHeader. width;
    bmp->height = bmpHeader.height;

    bmp->  redMask = colorInfo.  redMask;
    bmp->greenMask = colorInfo.greenMask;
    bmp-> blueMask = colorInfo. blueMask;
    bmp->alphaMask = colorInfo.alphaMask;

    fread(bmp->pixels, (size_t) (bmp->width*bmp->height), sizeof(uint32_t), file);

    fclose(file);
    return  true;
  fail:
    fclose(file);
    return false;
  }

  void Update$scalar(bmp *accum, const bmp *first, const bmp *second)
  {
    assert(accum);
    assert( first);
    assert(second);

    assert(first->width  == second->width );
    assert(first->height == second->height);

    assert(first->  redMask == second->  redMask);
    assert(first->greenMask == second->greenMask);
    assert(first-> blueMask == second-> blueMask);
    assert(first->alphaMask == second->alphaMask);

    int width  = first-> width;
    int height = first->height;

    accum->width  =  width;
    accum->height = height;

    accum->  redMask = first->  redMask;
    accum->greenMask = first->greenMask;
    accum-> blueMask = first-> blueMask;
    accum->alphaMask = first->alphaMask;

    free(accum->pixels);
    accum->pixels = (uint32_t *)
      calloc((size_t) (width*height), sizeof(uint32_t));
    assert(accum->pixels);

    for (int i = 0; i < height; ++i)
      for (int j = 0; j < width; ++j)
        {
          uint32_t  firstColor = *(first ->pixels + j + i*width);
          uint32_t secondColor = *(second->pixels + j + i*width);

          uint32_t color = (secondColor & second->alphaMask) ? secondColor : firstColor;

          *(accum->pixels + j + i*width) = color;
        }
  }

  void Update$vector(bmp *accum, const bmp *first, const bmp *second)
  {
    assert(accum);
    assert( first);
    assert(second);

    assert(first->width  == second->width );
    assert(first->height == second->height);

    assert(first->  redMask == second->  redMask);
    assert(first->greenMask == second->greenMask);
    assert(first-> blueMask == second-> blueMask);
    assert(first->alphaMask == second->alphaMask);

    int width  = first-> width;
    int height = first->height;

    accum->width  =  width;
    accum->height = height;

    accum->  redMask = first->  redMask;
    accum->greenMask = first->greenMask;
    accum-> blueMask = first-> blueMask;
    accum->alphaMask = first->alphaMask;

    free(accum->pixels);
    accum->pixels = (uint32_t *)
      calloc((size_t) (width*height), sizeof(uint32_t));
    assert(accum->pixels);

    for (int i = 0; i < height; ++i)
      for (int j = 0; j < width - 8; j += 8)
        {
          vector8u  firstColors = (vector8u)
            _mm256_loadu_si256((__m256i *) (first ->pixels + j + i*width));
          vector8u secondColors = (vector8u)
            _mm256_loadu_si256((__m256i *) (second->pixels + j + i*width));

          vector8u colors =
            (secondColors & second->alphaMask) ? secondColors : firstColors;

          _mm256_storeu_si256((__m256i *) (accum->pixels + j + i*width), (__m256i) colors);

        }
  }

}