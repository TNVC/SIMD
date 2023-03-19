#include "Model.h"

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <complex.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

const unsigned ITER_MAX = 50;
const unsigned WIDTH  = 1200;
const unsigned HEIGHT = 1000;

int main()
{
  sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "AVX");

  db::model::Model model{};
  db::model::CreateModel(&model, WIDTH, HEIGHT);

  bool isInterrupt = false;
  bool isVisible   =  true;

  bool *$statuses[] = { &isInterrupt, &isVisible };
  pthread_t thread{};
  pthread_create(
                 &thread,
                 nullptr,
                 [] (void *ptr) -> void *
                 {
                   bool **statuses = (bool **) ptr;
                   while(!*statuses[0])
                     switch (getchar())
                       {
                       case 'q':
                       case 'Q':
                       case EOF:
                         { *statuses[0] = !*statuses[0]; break; }
                       case 'p':
                       case 'P':
                         { *statuses[1] = !*statuses[1]; break; }
                       default: break;
                       }
                   return nullptr;
                 },
                 $statuses
                 );

  sf::Event event;
  sf::RectangleShape rect(sf::Vector2f(1.f, 1.f));
  for (int i = 0; i < 100; ++i)
    {
      printf("i:%d\n", i);
      while (window.pollEvent(event))
        if (event.type == sf::Event::Closed)
          { window.close(); goto end; }

      db::model::Update$AVX2_genData_float(&model, ITER_MAX);

      if (isVisible)
        {
          window.clear();

          for (unsigned y = 0; y < HEIGHT; ++y)
            for (unsigned x = 0; x < WIDTH; ++x)
              {
                uint32_t color = *(model.pixels + x + y*WIDTH);

                sf::Uint8 r = sf::Uint8 (color >> 24);
                sf::Uint8 g = sf::Uint8 (color >> 16);
                sf::Uint8 b = sf::Uint8 (color >>  8);
                sf::Uint8 a = sf::Uint8 (color >>  0);

                rect.setFillColor(sf::Color(r, g, b, a));

                rect.setPosition(sf::Vector2f((float)x, (float)y));
                window.draw(rect);
              }

          window.display();
        }
    }
 end:
  db::model::DestroyModel(&model);
  window.close();

  return 0;
}
