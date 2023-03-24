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
Множество Мандельброта (далее Set) задается преобразованием комплексной плоскости `φ: z -> z^2 + c`, где c - точка, к которой было применено преобразование. Все точки, которые при бесчисленном количестве приминения данного преобразования будут иметь модуль меньше или равный 1 и есть Set.
Цвет точки можно определить как скорость удаления от нуля.
В случае, если на каком либо этапе преобразований мы получим число с модулем большим или равным двум, то точка не предналежит Set, но для точек множества мы не можем ввести подобное условие.
Так как невозможно применить преобразование бесчисленное количество раз, то введем константу `MAX_ITERATION_COUNT = 50`, обозначающую максимальное количество итераций, после которого мы исходя из модуля определяем принадлежность точки множеству.

Рабочие размеры вычисляемой области 1200x1000.

Для краткости будет приведен только код обработки данных и его инфраструктура.
#### Реализация первая: использование встроенной поддержи комплексных чисел
> В данном разделе будут игноруроваться `complex float` и `complex double` в пользу `std::complex<float>` и `std::complex<double>`.
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

#### Реализация вторая: использование встроеной поддержки AVX2 для чисел с плавающей запятой
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
Составим таблицу по результатам тестирования
| Версия | Количество запусков | Общее время |
| ------ | ------ | ------ |
| Update$scalar      | 1024 | 1453442720 us | 
| Update$scalar      | 1024 | 1453366101 us | 
| Update$scalar      | 100  |  141617663 us | 
| Update$scalar      | 100  |  141819831 us | 
| Update$vector      | 1024 |   26849830 us |
| Update$vector      | 1024 |   27434100 us |
| Update$vector      | 100  |    2677489 us |
| Update$vector      | 100  |    2683991 us |
| Update$otherVector | 1000 |   60075043 us |
| Update$otherVector | 100  |    6046550 us |
| Update$otherVector | 100  |    6070999 us |

Как видно из данных векторная обработка быстрей скалярной, реализованной с помощью перегруженных опреторов, в 53.2 раза.
Но при использовании встраиваемых функций мы наблюдаем только прирост в 23.5 раз.
Как видно наша весторная реализация напротив реализации компилятора проигравает в 2.3 раза.

### _Глава вторая. Наложение изображений._
> Для наложения будет использоваться несжатый bmp формат с 32 битами на цвет.

Код наложения в силу очевидности представлен не будет.

Код тестировался на изображениях размером 3996x2997.

#### Итоги главы
Составим таблицу по результатам тестирования
| Версия | Количество запусков | Общее время |
| ------ | ------ | ------ |
| Update$scalar | 1024 | 57719105 us | 
| Update$scalar | 1024 | 57996748 us |
| Update$scalar | 1024 | 58071485 us | 
| Update$vector | 1024 | 26888798 us |
| Update$vector | 1024 | 26513938 us |
| Update$vector | 1024 | 25516790 us |

Как видно из данных, векторная реализация в среднем выполнялась в 2.2 раза быстрей скалярной.

Было сделано предположение, что вычисления не лимитирующий фактор данной задачи, а обращение к памяти. Было решено протестировать реализацию без обращения к памяти(работа с константными значениями**.

| Версия | Количество запусков | Общее время |
| ------ | ------ | ------ |
| Update$scalar | 1000000 | 3009951182 us |
| Update$scalar |  300000 |  903331335 us |
| Update$scalar |  300000 |  904145772 us |
| Update$scalar |  256000 |  772034926 us |
| Update$vector | 1000000 |  444471635 us |
| Update$vector |  300000 |  133279237 us |
| Update$vector |  300000 |  133572908 us |
| Update$vector |  256000 |  113926838 us |

В данном случае, векторная реализация быстрей скалярной в 6.8 раз , что доказывает, что лимитирующим фактором была работа с памятью.

#### Итоги
- Из данной работы мы поняли, что векторная обработка быстрей скалярной от 2.2 до 53.2 раз, в том числе, и в случаях, когда вычисления - не лимитирующий фактор.
- Большую эффективность векторной обработки над скалярной в расчете множества Мандельброта можно обусловить большей сложностью высчитывания скаляра по сравнению с наложением, а также не самой лучшей реализацией скалярного вычисления.
- Также мы научились применять средства векторной обработки компиляторного набора GCC.
