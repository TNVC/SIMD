#pragma GCC target("avx2")

#include "Model.h"

#include <x86intrin.h>

#include <stddef.h>
#include <assert.h>
#include <malloc.h>
#include <complex.h>

#include <stdio.h>

#define DUP8(VALUE)                                                         \
  { (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE), (VALUE) }
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

  typedef __v8sf vector8f;
  typedef __v8si vector8i;

  struct {
    float ReMin;
    float ReMax;
    float ImMin;
    float ImMax;
    float ReFactor;
    float ImFactor;
  } FloatSettings;

  static uint32_t GetColor(unsigned iterationCount, unsigned maxIterationCount);

  void CreateModel(Model *model, unsigned width, unsigned height)
  {
    assert(model);
    assert(width & height);

    model->pixels = (uint32_t *) calloc(width*height, sizeof(uint32_t));
    assert(model->pixels);

    model->width  =  width;
    model->height = height;

    float fwidth  = (float)  width;
    float fheight = (float) height;

    float ReMin = -2.0f;
    float ReMax =  1.0f;
    float ImMin = -1.2f;
    float ImMax =
      ImMin + (ReMax - ReMin)*fheight/fwidth;

    float ReFactor = (ReMax - ReMin)/(fwidth  - 1.f);
    float ImFactor = (ImMax - ImMin)/(fheight - 1.f);

    FloatSettings =
      {
        ReMin, ReMax,
        ImMin, ImMax,
        ReFactor, ImFactor
      };
  }

  void DestroyModel(Model *model)
  {
    assert(model);

    free(model->pixels);
    *model = {};

    FloatSettings = {};
  }

  static uint32_t GetColor(unsigned iterationCount, unsigned maxIterationCount)
  {
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

  void Update$scalar(Model *model, unsigned maxIterationCount)
  {
    assert(model);
    assert(maxIterationCount);

    auto transformation =
      [] (std::complex<float> z, std::complex<float> c)
      -> std::complex<float>
      { return z*z + c; };

    auto getModuleSquare =
      [] (std::complex<float> z) -> float
      {
        return z.real()*z.real() + z.imag()*z.imag();
      };

    unsigned width  = model-> width;
    unsigned height = model->height;
    for (unsigned i = 0; i < height; ++i)
      for (unsigned j = 0; j < width; ++j)
        {
          float x = FloatSettings.ReMin + FloatSettings.ReFactor*(float) j;
          float y = FloatSettings.ImMin + FloatSettings.ImFactor*(float) i;

          std::complex<float> z0(x, y);
          std::complex<float> z (x, y);
          unsigned iterationCount = 0;
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

  void Update$vector(Model *model, unsigned maxIterationCount)
  {
    assert(model);
    assert(maxIterationCount);
    
    unsigned width  = model-> width;
    unsigned height = model->height;
    for (unsigned i = 0; i < height; ++i)
      for (unsigned j = 0; j < width - 8; j += 8)
        {
          vector8f real = GEN_RE_FLOAT8(j);
          vector8f imag = GEN_IM_FLOAT8(i, j, width);
          vector8i iterationCount{};

          {
            const vector8f startReal{real};
            const vector8f startImag{imag};
            vector8f temp{};
            vector8i increment = DUP8(1);

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

          vector8f  factor{};
          vector8f rfactor = DUP8(1.f);

          {
            vector8f deliter = DUP8((float) maxIterationCount);
            factor = (vector8f)
              _mm256_cvtepi32_ps((__m256i) iterationCount)/deliter;
            rfactor -= factor;
          }

          vector8i color =
            (vector8i) _mm256_cvtps_epi32(9  *rfactor* factor* factor*factor*255)
            << 24 & 0xFF000000;
          color |=
            (vector8i) _mm256_cvtps_epi32(15 *rfactor*rfactor* factor*factor*255)
            << 16 & 0x00FF0000;
          color |=
            (vector8i) _mm256_cvtps_epi32(8.5*rfactor*rfactor*rfactor*factor*255)
            <<  8 & 0x0000FF00;
          color |= 0x000000FF;

          _mm256_storeu_ps((float *) (model->pixels + j + i*width), (__m256) color);
        }
  }

}
