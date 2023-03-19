#pragma GCC target("avx2")
//#pragma GCC optimize("O3")

#include "Model.h"

#include <x86intrin.h>

#include <stddef.h>
#include <assert.h>
#include <malloc.h>
#include <complex.h>

#include <stdio.h>

#define DUP8(VALUE)                                                         \
  { (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE) }
#define DUP16(VALUE)                                                        \
  { (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), \
    (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE) }
#define FIRST(VALUE)                                    \
  { (VALUE) + 0, (VALUE) + 1, (VALUE) + 2, (VALUE) + 3, \
    (VALUE) + 4, (VALUE) + 5, (VALUE) + 6, (VALUE) + 7 }
#define GEN_RE_FLOAT(VALUE)                                       \
  (FloatSettings.ReMin + FloatSettings.ReFactor*(float) (VALUE))
#define GEN_RE_FLOAT8(VALUE)                            \
  { GEN_RE_FLOAT(VALUE + 0), GEN_RE_FLOAT(VALUE + 1),   \
    GEN_RE_FLOAT(VALUE + 2), GEN_RE_FLOAT(VALUE + 3),   \
    GEN_RE_FLOAT(VALUE + 4), GEN_RE_FLOAT(VALUE + 5),   \
    GEN_RE_FLOAT(VALUE + 6), GEN_RE_FLOAT(VALUE + 7) }
#define GEN_IM_FLOAT(VALUE)                                       \
  (FloatSettings.ImMin + FloatSettings.ImFactor*(float) (VALUE))
#define OFFSET(VALUE, MAX_VALUE) (((VALUE) + 0) > (MAX_VALUE) ? 1 : 0)
#define GEN_IM_FLOAT8(H_VALUE, W_VALUE, WIDTH)                          \
  { GEN_IM_FLOAT(H_VALUE + OFFSET(W_VALUE + 0, WIDTH)),                 \
    GEN_IM_FLOAT(H_VALUE + OFFSET(W_VALUE + 1, WIDTH)),                 \
    GEN_IM_FLOAT(H_VALUE + OFFSET(W_VALUE + 2, WIDTH)),                 \
    GEN_IM_FLOAT(H_VALUE + OFFSET(W_VALUE + 3, WIDTH)),                 \
    GEN_IM_FLOAT(H_VALUE + OFFSET(W_VALUE + 4, WIDTH)),                 \
    GEN_IM_FLOAT(H_VALUE + OFFSET(W_VALUE + 5, WIDTH)),                 \
    GEN_IM_FLOAT(H_VALUE + OFFSET(W_VALUE + 6, WIDTH)),                 \
    GEN_IM_FLOAT(H_VALUE + OFFSET(W_VALUE + 7, WIDTH)) }

namespace db::model {

  typedef __v8sf  vector8fx32 ;
  typedef __v8si  vector8ix32 ;
  typedef __v16hi vector16hx16;

  typedef int16_t fixed;
  const size_t MSB =  4;
  const size_t LSB = 12;
  const fixed ONE = 1 << MSB;
  const vector16hx16 ONES = DUP16(ONE);

  struct {
    float ReMin;
    float ReMax;
    float ImMin;
    float ImMax;
    float ReFactor;
    float ImFactor;
  } FloatSettings;

  struct {
    fixed ReMin;
    fixed ReMax;
    fixed ImMin;
    fixed ImMax;
    fixed ReFactor;
    fixed ImFactor;
  } FixedSettings;

  static uint32_t GetColor(int iterationCount, int maxIterationCount);

  void CreateModel(Model *model, int width, int height)
  {
    assert(model);
    assert(width  > 0);
    assert(height > 0);

    size_t size = (size_t) width * (size_t) height;
    model->pixels = (uint32_t *) calloc(size, sizeof(uint32_t));
    assert(model->pixels);

    model->width  =  width;
    model->height = height;

    float fwidth  = (float)  width;
    float fheight = (float) height;

    float floatReMin = -2.0f;
    float floatReMax =  1.0f;
    float floatImMin = -1.2f;
    float floatImMax =
      floatImMin + (floatReMax - floatReMin)*fheight/fwidth;

    float floatReFactor = (floatReMax - floatReMin)/(fwidth  - 1.f);
    float floatImFactor = (floatImMax - floatImMin)/(fheight - 1.f);

    FloatSettings =
      {
        floatReMin, floatReMax,
        floatImMin, floatImMax,
        floatReFactor, floatImFactor
      };

    fixed fixedReMin = (fixed) -2.0f*ONE;
    fixed fixedReMax = (fixed)  1.0f*ONE;
    fixed fixedImMin = (fixed) -1.2f*ONE;
    fixed fixedImMax = (fixed)
      (fixedImMin + (fixedReMax - fixedReMin)*fheight/fwidth);

    fixed fixedReFactor = (fixed) ((fixedReMax - fixedReMin)/(fwidth  - 1.f));
    fixed fixedImFactor = (fixed) ((fixedImMax - fixedImMin)/(fheight - 1.f));

    FixedSettings =
      {
        fixedReMin, fixedReMax,
        fixedImMin, fixedImMax,
        fixedReFactor, fixedImFactor
      };
  }

  void DestroyModel(Model *model)
  {
    assert(model);

    free(model->pixels);
    *model = {};

    FloatSettings = {};
    FixedSettings = {};
  }

  static uint32_t GetColor(int iterationCount, int maxIterationCount)
  {
    assert(   iterationCount >= 0);
    assert(maxIterationCount >  0);

    double  factor = (double) iterationCount/maxIterationCount;
    double rfactor = 1. - factor;
    uint8_t r = (uint8_t) (9  *rfactor* factor* factor*factor*255);
    uint8_t g = (uint8_t) (15 *rfactor*rfactor* factor*factor*255);
    uint8_t b = (uint8_t) (8.5*rfactor*rfactor*rfactor*factor*255);

    return
      (r << 24 & 0xFF000000) |
      (g << 16 & 0x00FF0000) |
      (b <<  8 & 0x0000FF00) |
                 0x000000FF;
  }

  void Update$scalar(Model *model, int maxIterationCount)
  {
    assert(model);
    assert(maxIterationCount > 0);

    auto transformation =
      [] (std::complex<float> z, std::complex<float> c)
      -> std::complex<float>
      { return z*z + c; };

    auto getModuleSquare =
      [] (std::complex<float> z) -> float
      {
        return z.real()*z.real() + z.imag()*z.imag();
      };

    int width  = model-> width;
    int height = model->height;
    for (int i = 0; i < height; ++i)
      for (int j = 0; j < width; ++j)
        {
          float x = FloatSettings.ReMin + FloatSettings.ReFactor*(float) j;
          float y = FloatSettings.ImMin + FloatSettings.ImFactor*(float) i;

          std::complex<float> z0(x, y);
          std::complex<float> z (x, y);
          int iterationCount = 0;
          while (iterationCount < maxIterationCount)
            {
              if (getModuleSquare(z) >= 2.f*2.f) break;

              z = transformation(z, z0);

              ++iterationCount;
            }

          *(model->pixels + j + i*width) =
            GetColor(iterationCount, maxIterationCount);
        }
  }

  void Update$AVX2_genData_float(Model *model, int maxIterationCount)
  {
    assert(model);
    assert(maxIterationCount > 0);

    int width  = model-> width;
    int height = model->height;
    for (int i = 0; i < height; ++i)
      for (int j = 0; j < width - 8; j += 8)
        {
          vector8fx32 real = GEN_RE_FLOAT8(j);
          vector8fx32 imag = GEN_IM_FLOAT8(i, j, width);
          vector8ix32 iterationCount{};

          {
            const vector8fx32 startReal{real};
            const vector8fx32 startImag{imag};
            vector8fx32 temp{};
            vector8ix32 increment{ 1, 1, 1, 1, 1, 1, 1, 1 };

            while (_mm256_movemask_ps(iterationCount < maxIterationCount) == 0xFF)
              {
                increment &= (real*real + imag*imag < 2.f*2.f);
                if (_mm256_movemask_ps((__m256i) increment == 0) == 0xFF)
                  break;

                temp = real*real - imag*imag;
                imag = 2.f*real*imag;
                real = temp;

                real += startReal;
                imag += startImag;

                iterationCount += increment;
              }
          }

          vector8fx32  factor{};
          vector8fx32 rfactor = DUP8(1.f);

          {
            vector8fx32 deliter = DUP8((float) maxIterationCount);
            factor = (vector8fx32)
              _mm256_cvtepi32_ps((__m256i) iterationCount)/deliter;
            rfactor -= factor;
          }

          vector8ix32 color =
            (vector8ix32) _mm256_cvtps_epi32(9  *rfactor* factor* factor*factor*255)
            << 24 & 0xFF000000;
          color |=
            (vector8ix32) _mm256_cvtps_epi32(15 *rfactor*rfactor* factor*factor*255)
            << 16 & 0x00FF0000;
          color |=
            (vector8ix32) _mm256_cvtps_epi32(8.5*rfactor*rfactor*rfactor*factor*255)
            <<  8 & 0x0000FF00;
          color |= 0x000000FF;

          _mm256_storeu_ps((float *) (model->pixels + j + i*width), (__m256) color);
        }
  }

  void Update$AVX2_useData_float(Model *model, int maxIterationCount)
  {
    assert(model);
    assert(maxIterationCount > 0);

    size_t size = (size_t) model->width * (size_t) model->height;
    float *realMap = (float *) calloc(2*size, sizeof(float));
    assert(realMap);
    float *imagMap = realMap + size;

    int width  = model-> width;
    int height = model->height;
    for (int i = 0; i < height; ++i)
      for (int j = 0; j < width; ++j)
        {
          *(realMap + j + i*width) =
            {FloatSettings.ReMin + FloatSettings.ReFactor*(float) j};
          *(imagMap + j + i*width) =
            {FloatSettings.ImMin + FloatSettings.ImFactor*(float) i};
        }

    for (int i = 0; i < height; ++i)
      for (int j = 0; j < width - 8; j += 8)
        {
          vector8fx32 real = _mm256_loadu_ps(realMap + j + i*width);
          vector8fx32 imag = _mm256_loadu_ps(imagMap + j + i*width);
          vector8ix32 iterationCount{};

          {
            const vector8fx32 startReal{real};
            const vector8fx32 startImag{imag};
            vector8fx32 temp{};
            vector8ix32 increment = DUP8(1);

            while (_mm256_movemask_ps(iterationCount < maxIterationCount) == 0xFF)
              {
                increment &= (real*real + imag*imag < 2.f*2.f);
                if (_mm256_movemask_ps((__m256i) increment == 0) == 0xFF)
                  break;

                temp = real*real - imag*imag;
                imag = 2.f*real*imag;
                real = temp;

                real += startReal;
                imag += startImag;

                iterationCount += increment;
              }
          }

          vector8fx32  factor{};
          vector8fx32 rfactor = DUP8(1.f);

          {
            vector8fx32 deliter = DUP8((float) maxIterationCount);
            factor = (vector8fx32)
              _mm256_cvtepi32_ps((__m256i) iterationCount)/deliter;
            rfactor -= factor;
          }

          vector8ix32 color =
            (vector8ix32) _mm256_cvtps_epi32(9  *rfactor* factor* factor*factor*255)
            << 24 & 0xFF000000;
          color |=
            (vector8ix32) _mm256_cvtps_epi32(15 *rfactor*rfactor* factor*factor*255)
            << 16 & 0x00FF0000;
          color |=
            (vector8ix32) _mm256_cvtps_epi32(8.5*rfactor*rfactor*rfactor*factor*255)
            <<  8 & 0x0000FF00;
          color |= 0x000000FF;

          _mm256_storeu_ps((float *) (model->pixels + j + i*width), (__m256) color);
        }
    free(realMap);
  }

}
