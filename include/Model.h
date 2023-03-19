#pragma once

#include <stdint.h>

namespace db::model {

  struct Model {
    uint32_t *pixels;
    int  width;
    int height;
  };

  void  CreateModel(Model *model, int width, int height);
  void DestroyModel(Model *model);

  void Update$scalar(Model *model, int maxIterationCount);

  void Update$AVX2_genData_float(Model *model, int maxIterationCount);
  void Update$AVX2_useData_float(Model *model, int maxIterationCount);
  void Update$AVX2_genData_fixed(Model *model, int maxIterationCount);
  void Update$AVX2_useData_fixed(Model *model, int maxIterationCount);

}
