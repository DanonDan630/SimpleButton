# CH32 Button Inline Library

Небольшая `header-only` inline-библиотека на C для работы с кнопкой на микроконтроллерах CH32.

Библиотека умеет:

- инициализировать GPIO для кнопки
- работать с кнопками `ACTIVE_LOW` и `ACTIVE_HIGH`
- фильтровать дребезг контактов
- определять одиночный, двойной и множественные клики
- читать текущее состояние кнопки
- определять удержание
- работать через polling от миллисекундного таймера

Вся логика находится в `btn.h` и реализована через `static inline` функции. Отдельный `.c` файл для библиотеки компилировать не нужно.


## Быстрый Старт

Подключи библиотеку после заголовков CH32:

```c
#include "debug.h"
#include "btn.h"
```

Создай объект кнопки, указав GPIO-порт и пин:

```c
Button user_button = {GPIOA, GPIO_Pin_12};
```

Инициализируй кнопку:

```c
GPIO_InitTypeDef GPIO_InitStructure;

RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
buttonInit(&user_button, &GPIO_InitStructure, ACTIVE_LOW);
```

`ACTIVE_LOW` означает, что кнопка считается нажатой, когда GPIO читает `0`.
В этом режиме библиотека настраивает пин как `GPIO_Mode_IPU`.

`ACTIVE_HIGH` означает, что кнопка считается нажатой, когда GPIO читает `1`.
В этом режиме библиотека настраивает пин как `GPIO_Mode_IPD`.

Если не указать режим, по умолчанию будет использоваться `ACTIVE_LOW`:

```c
buttonInit(&user_button, &GPIO_InitStructure);
```

## Основной Цикл

Библиотека не использует прерывания для обработки кнопки. Нужно часто вызывать `tickButton()` в основном цикле и передавать текущее время в миллисекундах.

```c
while (1) {
    uint32_t now = millis();

    tickButton(&user_button, now);

    if (isPressed(&user_button)) {
        // кнопка сейчас нажата
    }

    if (isHeld(&user_button, now)) {
        // кнопка удерживается дольше HOLD_TIME_MS
    }
}
```

Функция `millis()` не входит в библиотеку. Минимальная реализация через SysTick показана ниже.

## Настройки

Значения таймингов по умолчанию задаются в `btn.h`:

```c
#define DEBOUNCE_TIME_MS 30
#define CLICKS_TIME_MS 500
#define HOLD_TIME_MS 1000
```

- `DEBOUNCE_TIME_MS` - сколько вход должен оставаться стабильным, чтобы состояние кнопки изменилось
- `CLICKS_TIME_MS` - временное окно для группировки кликов
- `HOLD_TIME_MS` - время удержания по умолчанию для `isHeld(btn, now)`

## API

### `buttonInit()`

Инициализирует GPIO-пин и сбрасывает внутреннее состояние кнопки.

```c
buttonInit(&user_button, &GPIO_InitStructure, ACTIVE_LOW);
buttonInit(&user_button, &GPIO_InitStructure, ACTIVE_HIGH);
buttonInit(&user_button, &GPIO_InitStructure); // по умолчанию ACTIVE_LOW
```

### `tickButton()`

Обновляет состояние debounce, считает клики и запоминает время нажатия.

```c
tickButton(&user_button, millis());
```

Лучше читать `millis()` один раз за итерацию цикла:

```c
uint32_t now = millis();
tickButton(&user_button, now);
```

### `isPressed()`

Возвращает текущее отфильтрованное состояние кнопки.

```c
if (isPressed(&user_button)) {
    // кнопка нажата
}
```

### `isHeld()`

Проверяет, удерживается ли кнопка достаточно долго.

С временем по умолчанию `HOLD_TIME_MS`:

```c
if (isHeld(&user_button, now)) {
    // удержание по времени по умолчанию
}
```

С явно заданным временем:

```c
if (isHeld(&user_button, now, 3000)) {
    // удержание 3000 мс
}
```

### `buttonGetClicks()`

Возвращает количество завершенных кликов.

```c
uint8_t clicks = buttonGetClicks(&user_button);

if (clicks == 1) {
    // одиночный клик
} else if (clicks == 2) {
    // двойной клик
} else if (clicks >= 3) {
    // несколько кликов
}
```

Если готового события клика нет, функция вернет `0`. После чтения событие клика очищается.

### `isClicked()` и `isDoubleClicked()`

Удобные функции для одиночного и двойного клика.

```c
if (isClicked(&user_button)) {
    // одиночный клик
}

if (isDoubleClicked(&user_button)) {
    // двойной клик
}
```

Эти функции тоже очищают готовое событие клика после чтения.

## Важный Момент Про Клики

`isClicked()`, `isDoubleClicked()` и `buttonGetClicks()` читают одно и то же внутреннее событие клика.

В реальной программе лучше выбрать один стиль:

```c
uint8_t clicks = buttonGetClicks(&user_button);
```

или:

```c
if (isClicked(&user_button)) {
}

if (isDoubleClicked(&user_button)) {
}
```

Не стоит ожидать, что одно и то же событие клика можно будет прочитать два раза.

## Полный Пример

Этот пример включает светодиод, пока кнопка удерживается дольше `LED_HOLD_MS`.
Одиночный клик инвертирует логику светодиода.

```c
#include "debug.h"
#include "btn.h"

#define LED_GPIO        GPIOA
#define LED_PIN         GPIO_Pin_8

#define BUTTON_GPIO     GPIOA
#define BUTTON_PIN      GPIO_Pin_12
#define BUTTON_MODE     ACTIVE_LOW
#define LED_HOLD_MS     500

static volatile uint32_t g_millis = 0;
static Button user_button = {BUTTON_GPIO, BUTTON_PIN};

void millis_init(void)
{
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    SysTick->SR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = (uint64_t)(SystemCoreClock / 1000U) - 1U;
    SysTick->CTLR = 0x0F;

    NVIC_SetPriority(SysTicK_IRQn, 1);
    NVIC_EnableIRQ(SysTicK_IRQn);
}

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    SysTick->SR = 0;
    g_millis++;
}

uint32_t millis(void)
{
    return g_millis;
}

static void led_set(uint8_t state)
{
    if (state) {
        GPIO_SetBits(LED_GPIO, LED_PIN);
    } else {
        GPIO_ResetBits(LED_GPIO, LED_PIN);
    }
}

static void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = LED_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIO, &GPIO_InitStructure);
    led_set(0);

    buttonInit(&user_button, &GPIO_InitStructure, BUTTON_MODE);
}

int main(void)
{
    uint8_t led_inverted = 0;

    GPIO_Config();
    millis_init();

    while (1) {
        uint32_t now = millis();

        tickButton(&user_button, now);

        if (isClicked(&user_button)) {
            led_inverted = !led_inverted;
        }

        led_set(isHeld(&user_button, now, LED_HOLD_MS) ^ led_inverted);
    }
}
```
