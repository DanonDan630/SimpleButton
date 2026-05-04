# CH32 Button Inline Library

A tiny header-only C button library for CH32 microcontrollers with debounce, click detection, and hold detection.

[Russian documentation](README_ru.md)

## License Recommendation

Use **GNU GPLv3** if you want projects that use this code to stay open source too.

GPLv3 is a strong copyleft license: if someone distributes a firmware, library, or other derivative work based on this code, they must provide the corresponding source code under GPLv3-compatible terms.

Recommended repository description:

```text
Tiny header-only C button library for CH32 microcontrollers with debounce, click, double-click, multi-click, and hold detection.
```

The library provides:

- GPIO button initialization
- active-low and active-high button modes
- software debounce
- single, double, and multi-click detection
- pressed state reading
- hold detection
- millisecond-timer based polling

All logic is implemented in `btn.h` as `static inline` functions, so there is no separate `.c` file to compile.


## Basic Usage

Include the library after your CH32 device/debug headers:

```c
#include "debug.h"
#include "btn.h"
```

Create a button object with GPIO port and pin:

```c
Button user_button = {GPIOA, GPIO_Pin_12};
```

Initialize the button:

```c
GPIO_InitTypeDef GPIO_InitStructure;

RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
buttonInit(&user_button, &GPIO_InitStructure, ACTIVE_LOW);
```

`ACTIVE_LOW` means the button is pressed when the GPIO input reads `0`.
The library configures the pin as `GPIO_Mode_IPU`.

`ACTIVE_HIGH` means the button is pressed when the GPIO input reads `1`.
The library configures the pin as `GPIO_Mode_IPD`.

If the mode argument is omitted, `ACTIVE_LOW` is used by default:

```c
buttonInit(&user_button, &GPIO_InitStructure);
```

## Main Loop Pattern

The library does not use interrupts for the button. Call `tickButton()` often from the main loop and pass the current time in milliseconds.

```c
while (1) {
    uint32_t now = millis();

    tickButton(&user_button, now);

    if (isPressed(&user_button)) {
        // button is currently pressed 
    }

    if (isHeld(&user_button, now)) {
        // button is held for HOLD_TIME_MS 
    }
}
```

The `millis()` function is not part of the library. A minimal SysTick-based implementation is shown below.

## Configuration

Default timing values are defined in `btn.h`:

```c
#define DEBOUNCE_TIME_MS 30
#define CLICKS_TIME_MS 500
#define HOLD_TIME_MS 1000
```

- `DEBOUNCE_TIME_MS` - input must stay stable for this time before state changes
- `CLICKS_TIME_MS` - time window for grouping clicks
- `HOLD_TIME_MS` - default hold time for `isHeld(btn, now)`

## API

### `buttonInit()`

Initializes the GPIO pin and resets internal button state.

```c
buttonInit(&user_button, &GPIO_InitStructure, ACTIVE_LOW);
buttonInit(&user_button, &GPIO_InitStructure, ACTIVE_HIGH);
buttonInit(&user_button, &GPIO_InitStructure); // defaults to ACTIVE_LOW 
```

### `tickButton()`

Updates debounce state, click counting, and press time.

```c
tickButton(&user_button, millis());
```

For cleaner code, read `millis()` once per loop:

```c
uint32_t now = millis();
tickButton(&user_button, now);
```

### `isPressed()`

Returns the current debounced button state.

```c
if (isPressed(&user_button)) {
    // button is pressed 
}
```

### `isHeld()`

Checks whether the button has been held long enough.

Uses `HOLD_TIME_MS`:

```c
if (isHeld(&user_button, now)) {
    // held for default time 
}
```

Uses a custom time:

```c
if (isHeld(&user_button, now, 3000)) {
    // held for 3000 ms
}
```

### `buttonGetClicks()`

Returns the number of completed clicks.

```c
uint8_t clicks = buttonGetClicks(&user_button);

if (clicks == 1) {
    // single click 
} else if (clicks == 2) {
    // double click 
} else if (clicks >= 3) {
    // multi-click 
}
```

This function returns `0` when no click event is ready. Reading clicks clears the ready click event.

### `isClicked()` and `isDoubleClicked()`

Convenience helpers for one-click and two-click events.

```c
if (isClicked(&user_button)) {
    // single click 
}

if (isDoubleClicked(&user_button)) {
    // double click 
}
```

These helpers also clear the ready click event after reading it.

## Click Reading Note

`isClicked()`, `isDoubleClicked()`, and `buttonGetClicks()` all read the same internal click event.

In real application code, prefer one style:

```c
uint8_t clicks = buttonGetClicks(&user_button);
```

or:

```c
if (isClicked(&user_button)) {
}

if (isDoubleClicked(&user_button)) {
}
```

Do not expect to read the same click event twice.

## Complete Example

This example turns the LED on while the button is held for `LED_HOLD_MS`.
A single click inverts the LED logic.

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
