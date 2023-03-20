#include "Model.h"
#include "ImageJoiner.h"

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <math.h>
#include <complex.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

const unsigned ITER_MAX = 50;
const unsigned WIDTH  = 1200;
const unsigned HEIGHT = 1000;
const unsigned TEST_COUNT = 1024;

void TestJoiningImages();
void TestMandelbrotSet();

void TestingMebdelbrotSet(
                          void (*funct)(db::model::Model *, unsigned),
                          db::model::Model *model,
                          sf::RenderWindow *window,
                          const char *name
                         );
void TestingJoiningImages(
                          void (*funct)(
                                        db::image::bmp *,
                                        const db::image::bmp *,
                                        const db::image::bmp *),
                          db::image::bmp * accum,
                          const db::image::bmp * first,
                          const db::image::bmp *second,
                          sf::RenderWindow *window,
                          const char *name
                         );
int main()
{
  printf("Chose test(Default: 1):\n1) Mandelbrot set\n2) Joining images\n");
  char code = 0;
  scanf(" %c", &code);
  if (code == '2')
    TestJoiningImages();
  else
    TestMandelbrotSet();
}

void TestJoiningImages()
{
  db::image::bmp coffee{};
  db::image::bmp  table{};
  db::image::bmp  accum{};
  db::image::CreateBmp(&coffee);
  if (!db::image::Load(&coffee , "./source/coffee.bmp"))
    {
      printf("Fail to load \"./source/coffee.bmp\".\n");
      return;
    }
  db::image::CreateBmp(&table );
  if (!db::image::Load(&table , "./source/table.bmp"))
    {
      printf("Fail to load \"./source/table.bmp\".\n");
      db::image::DestroyBmp(&coffee);
      return;
    }
  db::image::CreateBmp(&accum , &table);

  sf::RenderWindow window(sf::VideoMode(table.width, table.height), "AVX");

  TestingJoiningImages(db::image::Update$scalar, &accum, &table, &coffee, &window, "scalar");
  TestingJoiningImages(db::image::Update$vector, &accum, &table, &coffee, &window, "vector");

  db::image::DestroyBmp(&table );
  db::image::DestroyBmp(&coffee);
  db::image::DestroyBmp(&accum );
  window.close();
}

void TestMandelbrotSet()
{
  sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "AVX");

  db::model::Model model{};
  db::model::CreateModel(&model, WIDTH, HEIGHT);

  TestingMebdelbrotSet(db::model::Update$scalar, &model, &window, "scalar");
  TestingMebdelbrotSet(db::model::Update$vector, &model, &window, "vector");

  db::model::DestroyModel(&model);
  window.close();
}

void TestingMebdelbrotSet(
                          void (*funct)(db::model::Model *, unsigned),
                          db::model::Model *model,
                          sf::RenderWindow *window,
                          const char *name
                         )
{
  printf("Skip %s test?(y,N):\n", name);
  char code = 0;
  scanf(" %c", &code);
  if (code == 'y') return;

  bool isVisible = false;
  sf::RectangleShape rect(sf::Vector2f(1.f, 1.f));
  sf::Event event{};
  timeval stop{}, start{};

  printf("Start %s test.\n", name);
  gettimeofday(&start, NULL);
  for (unsigned i = 0; i < TEST_COUNT; ++i)
    {
      while (window->pollEvent(event))
        if (event.type == sf::Event::KeyPressed)
          if (event.key.code == sf::Keyboard::Escape)
            isVisible = !isVisible;

      funct(model, ITER_MAX);
      window->clear();
      if (isVisible)
        for (unsigned y = 0; y < HEIGHT; ++y)
          for (unsigned x = 0; x < WIDTH; ++x)
            {
              rect.setFillColor(sf::Color(*(model->pixels + x + y*WIDTH)));
              rect.setPosition(sf::Vector2f((float)x, (float)y));
              window->draw(rect);
            }
      window->display();
    }
  gettimeofday(&stop, NULL);

  printf("End %s test. Time: %ld us\n", name,
         (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
}

void TestingJoiningImages(
                          void (*funct)(
                                              db::image::bmp *,
                                        const db::image::bmp *,
                                        const db::image::bmp *),
                                db::image::bmp * accum,
                          const db::image::bmp * first,
                          const db::image::bmp *second,
                          sf::RenderWindow *window,
                          const char *name
                         )
{
  printf("Skip %s test?(y,N):\n", name);
  char code = 0;
  scanf(" %c", &code);
  if (code == 'y') return;

  bool isVisible = false;
  sf::RectangleShape rect(sf::Vector2f(1.f, 1.f));
  sf::Event event{};
  timeval stop{}, start{};

  printf("Start %s test.\n", name);
  gettimeofday(&start, NULL);
  for (unsigned i = 0; i < TEST_COUNT; ++i)
    {
      while (window->pollEvent(event))
        if (event.type == sf::Event::KeyPressed)
          if (event.key.code == sf::Keyboard::Escape)
            isVisible = !isVisible;

      funct(accum, first, second);
      window->clear();
      if (isVisible)
        for (unsigned y = 0; y < accum->height; ++y)
          for (unsigned x = 0; x < accum->width; ++x)
            {
              uint32_t color = *(accum->pixels + x + y*WIDTH);
              sf::Uint8 r = (sf::Uint8) ((color & accum->  redMask) >> 16);
              sf::Uint8 g = (sf::Uint8) ((color & accum->greenMask) >>  8);
              sf::Uint8 b = (sf::Uint8) ((color & accum-> blueMask) >>  0);
              sf::Uint8 a = (sf::Uint8) ((color & accum->alphaMask) >> 24);

              rect.setFillColor(sf::Color(r, g, b, a));
              rect.setPosition(sf::Vector2f((float)x, (float)y));
              window->draw(rect);
            }
      window->display();
    }
  gettimeofday(&stop, NULL);

  printf("End %s test. Time: %ld us\n", name,
         (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
}
