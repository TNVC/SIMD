#pragma once

#include <stdint.h>

namespace db::image {

  struct bmp {
    uint32_t *pixels;
    unsigned  width;
    unsigned height;
    uint32_t   redMask;
    uint32_t greenMask;
    uint32_t  blueMask;
    uint32_t alphaMask;
  };

  void  CreateBmp(bmp *bmp);
  void  CreateBmp(bmp *bmp, unsigned width, unsigned height);
  void DestroyBmp(bmp *bmp);

  bool Load(bmp *bmp, const char *path);

  void Update$scalar(bmp *accum, const bmp *first, const bmp *second);
  void Update$vector(bmp *accum, const bmp *first, const bmp *second);

}
