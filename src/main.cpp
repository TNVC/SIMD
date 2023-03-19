#include "Model.h"
#include "ImageJoiner.h"

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

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

int main()
{
  printf("Chose test(Default: 1):\n1) Mandelbrot set\n2) Joining images\n");
  if (getchar() == '2')
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
  if (db::image::Load(&coffee , "./source/coffee.bmp"))
    {
      printf("Fail to load \"./source/coffee.bmp\".\n");
      return;
    }
  db::image::CreateBmp(&table );
  if (db::image::Load(&table , "./source/table.bmp"))
    {
      printf("Fail to load \"./source/table.bmp\".\n");
      db::image::DestroyBmp(&coffee);
      return;
    }
  db::image::CreateBmp(&accum , table.width, table.height);

  sf::RenderWindow window(sf::VideoMode(table.width, table.height), "AVX");

  bool isVisible = true;
  sf::RectangleShape rect(sf::Vector2f(1.f, 1.f));
  sf::Event event{};
  timeval stop{}, start{};

  printf("Start scalar test.\n");
  gettimeofday(&start, NULL);
  for (int i = 0; i < TEST_COUNT; ++i)
    {
      while (window.pollEvent(event))
        if (event.type == sf::Event::KeyPressed)
          if (event.key.code == sf::Keyboard::Escape)
            isVisible = !isVisible;

      db::image::Update$scalar(&accum, &table, &coffee);
      window.clear();
      if (isVisible)
        for (unsigned y = 0; y < table.height; ++y)
          for (unsigned x = 0; x < table.width; ++x)
            {
              rect.setFillColor(sf::Color(*(accum.pixels + x + y*WIDTH)));
              rect.setPosition(sf::Vector2f((float)x, (float)y));
              window.draw(rect);
            }
      window.display();
    }
  gettimeofday(&stop, NULL);

  printf("End scalar test. Time: %ld us\n",
         (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);

  printf("Start vector test.\n");
  gettimeofday(&start, NULL);
  for (int i = 0; i < TEST_COUNT; ++i)
    {
      while (window.pollEvent(event))
        if (event.type == sf::Event::KeyPressed)
          if (event.key.code == sf::Keyboard::Escape)
            isVisible = !isVisible;

      db::image::Update$vector(&accum, &table, &coffee);
      window.clear();
      if (isVisible)
        for (unsigned y = 0; y < table.height; ++y)
          for (unsigned x = 0; x < table.width; ++x)
            {
              rect.setFillColor(sf::Color(*(accum.pixels + x + y*WIDTH)));
              rect.setPosition(sf::Vector2f((float)x, (float)y));
              window.draw(rect);
            }
      window.display();
    }
  gettimeofday(&stop, NULL);

  printf("End vector test. Time: %ld us\n",
         (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);

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

  bool isVisible = true;
  sf::RectangleShape rect(sf::Vector2f(1.f, 1.f));
  sf::Event event{};
  timeval stop{}, start{};

  printf("Start scalar test.\n");
  gettimeofday(&start, NULL);
  for (int i = 0; i < TEST_COUNT; ++i)
    {
      while (window.pollEvent(event))
        if (event.type == sf::Event::KeyPressed)
          if (event.key.code == sf::Keyboard::Escape)
            isVisible = !isVisible;

      db::model::Update$scalar(&model, ITER_MAX);
      window.clear();
      if (isVisible)
        for (unsigned y = 0; y < HEIGHT; ++y)
          for (unsigned x = 0; x < WIDTH; ++x)
            {
              rect.setFillColor(sf::Color(*(model.pixels + x + y*WIDTH)));
              rect.setPosition(sf::Vector2f((float)x, (float)y));
              window.draw(rect);
            }
      window.display();
    }
  gettimeofday(&stop, NULL);

  printf("End scalar test. Time: %ld us\n",
         (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);

  printf("Start vector test.\n");
  gettimeofday(&start, NULL);
  for (int i = 0; i < TEST_COUNT; ++i)
    {
      while (window.pollEvent(event))
        if (event.type == sf::Event::KeyPressed)
          if (event.key.code == sf::Keyboard::Escape)
            isVisible = !isVisible;

      db::model::Update$vector(&model, ITER_MAX);
      window.clear();
      if (isVisible)
        for (unsigned y = 0; y < HEIGHT; ++y)
          for (unsigned x = 0; x < WIDTH; ++x)
            {
              rect.setFillColor(sf::Color(*(model.pixels + x + y*WIDTH)));
              rect.setPosition(sf::Vector2f((float)x, (float)y));
              window.draw(rect);
            }
      window.display();
    }
  gettimeofday(&stop, NULL);

  printf("End vector test. Time: %ld us\n",
         (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);

  db::model::DestroyModel(&model);
  window.close();
}
