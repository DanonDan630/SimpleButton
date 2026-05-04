#include "debug.h"

// It is necessary to include debug.h before btn.h

#include "btn.h"

static volatile uint32_t g_millis = 0;

// Millisecond timer used by tickButton() and isHeld().
void millis_init(void){
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
void SysTick_Handler(void){
    SysTick->SR = 0;
    g_millis++;
}

uint32_t millis(void)
{
    return g_millis;
}

// Create Button object
Button user_button = {GPIOA, GPIO_Pin_12};

static void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // LED on PA8, used to show results of button checks
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOA, GPIO_Pin_8);


    // buttonInit(Button *btn,
    //              GPIO_InitTypeDef *gpio_init,
    //              ButtonModes mode)
    //                                   
    // Configures the GPIO pin and resets button state
    // 
    // ACTIVE_LOW  means pressed = pin reads 0, so the library uses IPU
    // ACTIVE_HIGH means pressed = pin reads 1, so the library uses IPD

    buttonInit(&user_button, &GPIO_InitStructure, ACTIVE_LOW);

    // If mode is omitted, ACTIVE_LOW is used by default:
    //
    // buttonInit(&user_button, &GPIO_InitStructure);
    
}

static void led_on(void)
{
    GPIO_SetBits(GPIOA, GPIO_Pin_8);
}

static void led_off(void)
{
    GPIO_ResetBits(GPIOA, GPIO_Pin_8);
}

static void example_pressed_and_held(Button *btn, uint32_t now)
{
    
    // isPressed() returns the current debounced button state
    // It is true while the button is physically held down

    if (isPressed(btn)) {
        led_on();
    } else {
        led_off();
    }

    
    // isHeld(btn, now) uses HOLD_TIME_MS from btn.h
    // isHeld(btn, now, ms) uses a custom hold time
    
    if (isHeld(btn, now)) {
        led_on();
    }

    if (isHeld(btn, now, 3000)) {
        led_off();
    }
}

static void example_click_helpers(Button *btn)
{
    
    // isClicked() returns true once after one completed click
    // It clears the ready click event after reading it
    
    if (isClicked(btn)) {
        led_on();
    }
    
    // isDoubleClicked() returns true once after two clicks in CLICKS_TIME_MS
    // It also clears the ready click event after reading it
    
    if (isDoubleClicked(btn)) {
        led_off();
    }
}

static void example_click_counter(Button *btn)
{

    // buttonGetClicks() is the most general click reader
    // It returns 0 when no click event is ready, or 1, 2, 3... when ready
    // Use it instead of several isClicked style checks in real code

    uint8_t clicks = buttonGetClicks(btn);
    if (clicks == 1) {
        led_on();
    } else if (clicks == 2) {
        led_off();
    } else if (clicks >= 3) {
        led_on();
    }
}

int main(void)
{
    GPIO_Config();
    millis_init();

    while (1) {
        uint32_t now = millis();

        // tickButton() must be called often
        // It updates debounce state, click counting, and press time

        tickButton(&user_button, now);


        // Pick one click-reading style in a real program:
        // example_click_helpers() or example_click_counter()
        // Both clear the same click event after reading it

        example_pressed_and_held(&user_button, now);
        example_click_counter(&user_button);

        // Alternative:
        // example_click_helpers(&user_button);

    }
}
