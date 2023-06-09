#  Изучение SIMD(AVX2) расширения архитектуры x86-64
**Выполнил Буторин Даниил**
## Цель:
    Изучить  возможности компиляторов набора GCC  к  intrinsic  функциям и
    использованию набора команд для векторов; применить их путем отрисовки
    множества  Мандельброта   и   наложения  изображений  формата  ".bmp".
## В работе используются:
    Язык программирования C\C++; набор  компиляторов GCC; инструмент
    callgring  утилиты valgring; инструмент визуализации KCachegring.
## Экспериментальная установка:
    Ноутбук  фирмы   "Honor"  на  процессоре "AMD  Ryzen  5  5500U
    with Radeon Graphics"  с OS "GNU/Linux 22.04.1-Ubuntu x86_64".
## Теоретическая справка:
SIMD(single instruction, multiple data) - принцип компьютерных вычислений, позволяющий обеспечить параллелизм на уровне данных. Процессоры архитектуры x86-64 имеют поддержку SIMD инструкций в виде наборов инструкций SSE, SSE2, SSE3, AVX, AVX2, AVX512 и прочие. В настоящее время, большинство стационарных процессоров x86-64 поддерживают набор инструкций AVX2.
Intrinsic - built-in(встраиваемые) функции, реализация которых зашита в компиляторе. В ассемблерном коде, они заменяются на одну или несколько команд, вместо вызова функций. Основное отличие built-in функций от ассемблерных вставок - оптимизация компилятора не отключается. Примером intrinsic функции может послужить `memset()`, которая на x86-64 заменяется на `rep stosb`.
## Ход работы:
> В работе для визуализации  информации будет использоваться библиотека графического интерфейса SFML.
> Так как внедрение векторной обработки в вывод изображения не будет реализовано в данной работе, то в финальные результаты оно входить не будет.

### _Глава первая. Множество Мандельброта._
Множество Мандельброта (далее Set) задается преобразованием комплексной плоскости `φ: z -> z^2 + c`, где c - точка, к которой было применено преобразование. Все точки, которые при бесчисленном количестве применения данного преобразования будут иметь модуль меньше или равный 1 и есть Set.
Цвет точки можно определить как скорость удаления от нуля.
В случае, если на каком либо этапе преобразований мы получим число с модулем большим или равным двум, то точка не предналежит Set, но для точек множества мы не можем ввести подобное условие.
Так как невозможно применить преобразование бесчисленное количество раз, то введем константу `MAX_ITERATION_COUNT = 50`, обозначающую максимальное количество итераций, после которого мы исходя из модуля определяем принадлежность точки множеству.

Рабочие размеры вычисляемой области 1200x1000.

Для краткости будет приведен только код обработки данных и его инфраструктура.
#### Реализация первая: использование встроенной поддержи комплексных чисел
> В данном разделе будут игнорироваться `complex float` и `complex double` в пользу `std::complex<float>` и `std::complex<double>`.
```clike
float ReMin;
float ReMax;
float ImMin;
float ImMax;
float ReFactor;
float ImFactor;

typedef uint32_t Color;

struct Model {
  size_t  width;
  size_t height;
  Color *pixels;
};

void Update$scalar(Model *model);
void Update$vector(Model *model);
```

```clike
void Update$scalar(Model *model)
{
    auto transform =
      [] (std::complex<float> z, std::complex<float> c)
      -> std::complex<float>
      { return z*z + c; };
    auto getSquareOfModule =
      [] (std::complex<float> z) -> float
    {
      return z.real()*z.real() + z.imag()*z.imag();
    };

    for (size_t i = 0; i < model->height; ++i)
      for (size_t j = 0; j < model->width; ++j)
      {
          flaot x = ReMin + ReFactor*j;
          flaot y = ImMin + ImFactor*i;

          std::complex<float> z0(x, y);
          std::complex<float> z (x, y);

          size_t iterationCount = 0;
          while (iterationCount < MAX_ITERATION_COUNT)
          {
              if (getSquareOfModule(z) > 4.f)
                  break;

              z = transform(z, z0);

              ++iterationCount;
          }

          float  factor = iterationCount / MAX_ITERATION_COUNT;
          float rfactor = 1 - factor;
          uint8 r = 9  *rfactor* factor* factor*factor*255;
          uint8 g = 15 *rfactor*rfactor* factor*factor*255;
          uint8 b = 8.5*rfactor*rfactor*rfactor*factor*255;

          model[j][i] = Color(r, g, b);
      }
}
```

Также была реализована векторная функция с прямым использованием встраиваемых функций.

#### Реализация вторая: использование встроенной поддержки AVX2 для чисел с плавающей запятой
```clike
void Update$vector(Model *model)
{
    for (size_t i = 0; i < model->height; ++i)
      for (size_t j = 0; j < model->width - 8; j += 8)
      {
          vector8f real = Vector
              (j, 8, [](float val)->float{ return ReMin + ReFactor*val; });
          vector8f imag = Vector
              (i, 8, [](float val)->float{ return ImMin + ImFactor*val; });

          vector8f startReal = real;
          vector8f startImag = imag;

          vector8i increment = Vector(1);

          vector8u iterationCount = Vector(0);
          while (Vector(iterationCount < MAX_ITERATION_COUNT).hasnt(true))
          {
              increment &= Vector(real*real + imag*imag > 4.f);
              if (increment.all(0))
                  break;

              vector8f temp = real*real - imag*imag;
              imag = 2.f*real*imag;
              real = temp;

              iterationCount += increnent;
          }

          vector8f factor = iterationCount / MAX_ITERATION_COUNT;
          vector8f rfactor = 1 - factor;
          vector8u r = 9  *rfactor* factor* factor*factor*255;
          vector8u g = 15 *rfactor*rfactor* factor*factor*255;
          vector8u b = 8.5*rfactor*rfactor*rfactor*factor*255;

          Vector.store(&model[j][i], Color(r, g, b));
      }
}
```

#### Итоги главы
Составим графики по результатам тестирования.

![alt text](https://github.com/TNVC/SIMD/blob/master/source/plot0.png?raw=true)
![alt text](https://github.com/TNVC/SIMD/blob/master/source/plot1.png?raw=true)
![alt text](https://github.com/TNVC/SIMD/blob/master/source/plot2.png?raw=true)

Для начало сравним следующие версии: скалярная и векторная с использованием перегруженных операторов, реализация которых зашита в компиляторе.
Видно, что в таком сравнении прирост скорости составляет (53.2 +- 0.08) раза.
Также была реализованна версия с явным использование встраиваемых функций(то есть без перегруженных операторов).
Если ее сравнивать со скалярной, то прирост скорости составляет (23.5 +- 0.07) раз.
Данная разница в приростах между векторными версиями(2.3 раза) может быть обусловлена фактом, что компилятор лучше оптимизирует вычисления по сравнению с человеком(векторная реализация с прямым использованием встраиваемых функций никак не заменяла порядок вычислений).
Из данных наблюдений можно сделать следующий вывод: надо помогать компилятору оптимизировать код(например: добавлять работу с векторами), но не делать всю работу за него(например: организовать порядок вычислений сложного арифметического выражения).

### _Глава вторая. Наложение изображений._
> Для наложения будет использоваться несжатый bmp формат с 32 битами на цвет.

Код наложения в силу очевидности представлен не будет.

Код тестировался на изображениях размером 3996x2997.

#### Итоги главы
Составим графики по результатам тестирования.

![alt text](https://github.com/TNVC/SIMD/blob/master/source/plot4.png?raw=true)

В данном случае сравнивая скалярную версию мы видим прирост скорости в (4.3 +- 0.11) раза.
Малый прирост скорости можно обусловить тем, что лимитирующий фактор - работа с памятью, но не вычисления.
Для проверки предположения было проведено сравнение двух версий с увеличенным весом вычислений на фоне работы с памятью(для каждого обращения к памяти - 1000 вычислений).

То есть, данный код
```clike=
    Color  firstColor = first ->pixels[i][j];
    Color secondColor = second->pixels[i][j];

    float alpha = secondColor.alpha / 255.f;
    Color color = secondColor*alpha + firstColor*(1 - alpha);

    accum->pixels[i][j] = color*255.f;
```
был заменен на
```clike=
    Color  firstColor = first ->pixels[i][j];
    Color secondColor = second->pixels[i][j];

    Color color{};
    for (int i = 0; i < 1000; ++i)
      {
        float alpha = secondColor.alpha / 255.f;
        color = secondColor*alpha + firstColor*(1 - alpha);
      }

    accum->pixels[i][j] = color*255.f;
```

Составим графики по результатам тестирования.

![alt text](https://github.com/TNVC/SIMD/blob/master/source/plot5.png?raw=true)

В данном случае, векторная реализация быстрей скалярной в (8.4 +- 0.17) раза.
Из данных результатов можно сделать вывод, что более значимым лимитирующим фактором была работа с памятью, чем вычисления.

#### Итоги
Из данной работы мы поняли:
- Векторная обработка быстрей скалярной от 4.3 до 53.2 раз, в том числе, и в случаях, когда вычисления - не лимитирующий фактор.
- Надо помогать компилятору оптимизировать код, но не делать всю работу за него.
- Вычисления - не всегда лимитирующий фактор.
- Также мы научились применять средства векторной обработки компиляторного набора GCC.
