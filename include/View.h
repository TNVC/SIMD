#if 0

#pragma once

#include "Model.h"

namespace db::gui {

  void Initialize(unsigned width, unsigned height);
  void   Finalize();

  unsigned GetWidth ();
  unsigned GetHeight();

  void Draw(const model::Model *model);

}
#endif
