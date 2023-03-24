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

#define GET_COLOR(V0, V1, V2, V3, V4, V5)                               \
  do                                                                    \
    {                                                                   \
      vector8f faccum = DUP8(V0);                                       \
      vector8i iaccum = DUP8(0);                                        \
      vector8f _const = DUP8(255.f);                                    \
      vector8i shift = DUP8(V5);                                        \
      vector8u mask = DUP8((unsigned) (0xFF << V5));                    \
      faccum = _mm256_mul_ps(faccum, V1);                               \
      faccum = _mm256_mul_ps(faccum, V2);                               \
      faccum = _mm256_mul_ps(faccum, V3);                               \
      faccum = _mm256_mul_ps(faccum, V4);                               \
      faccum = _mm256_mul_ps(faccum, _const);                           \
      iaccum = (vector8i) _mm256_cvtps_epi32((__m256) faccum);          \
      iaccum = (vector8i)                                               \
        _mm256_sllv_epi32((__m256i) iaccum, (__m256i) shift);           \
      iaccum = (vector8i)                                               \
        _mm256_and_si256((__m256i) iaccum, (__m256i) mask);             \
      color = (vector8i)                                                \
        _mm256_or_si256((__m256i) color, (__m256i) iaccum);             \
    } while (0)

namespace db::model {

  typedef int16_t fixed;
  const size_t WEIGTH = 12;
  const fixed ONE = 1 << WEIGTH;
#define NEGATIVE(VALUE) ((fixed) (VALUE | 0x8000))

  typedef __v8sf  vector8f ;
  typedef __v8si  vector8i ;
  typedef __v8su  vector8u ;
  typedef __v16hi vector16h;

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

    fixed fixedReMin = (fixed) (-2  *ONE);
    fixed fixedReMax = (fixed) ( 1  *ONE);
    fixed fixedImMin = (fixed) (-1.2*ONE);
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
            vector8f value = DUP8(2.f);
            vector8f sqValue = DUP8(2.f*2.f);
            vector8i increment = DUP8(1);
            vector8i zeros = DUP8(0);

            while (_mm256_movemask_ps(iterationCount < maxIterationCount) == 0xFF)
              {
                // increment &= real*real + imag*imag < 2.f*2.f _CMP_LT_OQ
                temp = _mm256_cmp_ps(
                                     _mm256_add_ps(
                                                   _mm256_mul_ps(real, real),
                                                   _mm256_mul_ps(imag, imag)
                                                  ),
                                     sqValue,
                                     _CMP_LT_OQ
                                    );
                increment = (vector8i) _mm256_and_ps((__m256) increment, temp);

                temp = (vector8f) _mm256_cmpeq_epi32((__m256i) increment, (__m256i) zeros);
                if (_mm256_movemask_ps((__m256) temp) == 0xFF)
                  break;

                //temp = real*real - imag*imag;
                temp =
                  _mm256_sub_ps(_mm256_mul_ps(real, real), _mm256_mul_ps(imag, imag));
                //imag = 2.f*real*imag;
                imag = _mm256_mul_ps(value, _mm256_mul_ps(real, imag));
                real = temp;

                //real += startReal;
                //imag += startImag;
                real = _mm256_add_ps(real, startReal);
                imag = _mm256_add_ps(imag, startImag);

                //iterationCount += increment;
                iterationCount = (vector8i)
                  _mm256_add_ps((__m256) iterationCount, (__m256) increment);
              }
          }

          vector8f  factor{};
          vector8f rfactor = DUP8(1.f);

          {
            vector8f deliter = DUP8((float) maxIterationCount);
            //factor = (vector8f)
            //  _mm256_cvtepi32_ps((__m256i) iterationCount)/deliter;
            factor = (vector8f)
              _mm256_div_ps(_mm256_cvtepi32_ps((__m256i) iterationCount), deliter);
            //rfactor -= factor;
            rfactor = _mm256_sub_ps(rfactor, factor);
          }

          vector8i color{};
          //color =  (vector8i)
          //  _mm256_cvtps_epi32(9  *rfactor* factor* factor*factor*255)
          //<< 24 & 0xFF000000;
          GET_COLOR(9.0, rfactor,  factor, factor, factor, 24);
          //color |=
          //  (vector8i) _mm256_cvtps_epi32(15 *rfactor*rfactor* factor*factor*255)
          //  << 16 & 0x00FF0000;
          GET_COLOR(15., rfactor, rfactor, factor, factor, 16);
          //color |=
          //  (vector8i) _mm256_cvtps_epi32(8.5*rfactor*rfactor*rfactor*factor*255)
          //  <<  8 & 0x0000FF00;
          GET_COLOR(8.5, rfactor, rfactor,rfactor, factor,  8);
          //color |= 0x000000FF;
          {
            vector8u mask = DUP8(0x000000FF);
            color = (vector8i) _mm256_or_si256((__m256i) color, (__m256i) mask);
          }
          _mm256_storeu_ps((float *) (model->pixels + j + i*width), (__m256) color);
        }
  }

}
