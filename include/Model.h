#pragma once

#include <stdint.h>

namespace db::model {

  struct Model {
    uint32_t *pixels;
    unsigned  width;
    unsigned height;
  };

  void  CreateModel(Model *model, unsigned width, unsigned height);
  void DestroyModel(Model *model);

  void Update$scalar(Model *model, unsigned maxIterationCount);
  void Update$vector(Model *model, unsigned maxIterationCount);
}
