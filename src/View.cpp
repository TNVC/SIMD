#if 0

#include "View.h"

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <pthread.h>

namespace db::gui {

  static unsigned  Width;
  static unsigned Height;

  static sf::RenderWindow Window;

  void Initialize(unsigned width, unsigned height)
  {
    Width  =  width;
    Height = height;
  }

  void Finalize() {}

  unsigned GetWidth () { return  Width; }
  unsigned GetHeight() { return Height; }

  void Draw(const model::Model *model) {}

}
#endif
