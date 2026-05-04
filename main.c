#include "debug.h"
#include "SimpleButton/btn.h"

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
