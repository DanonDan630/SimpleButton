#ifndef BTN_H
#define BTN_H


#define DEBOUNCE_TIME_MS 30
#define CLICKS_TIME_MS 500
#define HOLD_TIME_MS 1000

#define BTN_GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define isHeld(...) BTN_GET_MACRO(__VA_ARGS__, isHeld3, isHeld2)(__VA_ARGS__)
#define isHeld2(btn, now) isHeldImpl(btn, now, HOLD_TIME_MS)
#define isHeld3(btn, now, hold_time_ms) isHeldImpl(btn, now, hold_time_ms)
#define buttonInit(...) BTN_GET_MACRO(__VA_ARGS__, buttonInit3, buttonInit2)(__VA_ARGS__)
#define buttonInit2(btn, gpio_init) buttonInitImpl(btn, gpio_init, ACTIVE_LOW)
#define buttonInit3(btn, gpio_init, mode) buttonInitImpl(btn, gpio_init, mode)


typedef enum {
    ACTIVE_LOW,
    ACTIVE_HIGH,
} ButtonModes;

typedef enum {
    RELEASED,
    PRESSED
} ButtonStates;


typedef struct {
    GPIO_TypeDef *gpiox;
    uint16_t pin;
    ButtonModes mode;
    uint32_t last_time;
    uint32_t last_click_time;
    uint32_t press_time;
    
    uint8_t last_raw;
    uint8_t stable;
    uint8_t click_count;
    uint8_t click_ready;
    uint8_t waiting_clicks;
    uint8_t held;
} Button;

static inline uint8_t getRawButtonState(Button *btn){
    uint8_t level = GPIO_ReadInputDataBit(btn -> gpiox, btn -> pin);
    return (btn -> mode == ACTIVE_LOW) ? !level : level;
}

static inline void buttonInitImpl(Button *btn, GPIO_InitTypeDef *gpio_init, ButtonModes mode){
    btn -> mode = mode;
    gpio_init -> GPIO_Pin = btn -> pin;
    gpio_init -> GPIO_Mode = (mode == ACTIVE_LOW) ? GPIO_Mode_IPU : GPIO_Mode_IPD;
    GPIO_Init(btn -> gpiox, gpio_init);
    btn -> last_time = 0;
    btn -> click_count = 0;
    btn -> click_ready = 0;
    btn -> last_raw = getRawButtonState(btn);
    btn -> stable = btn -> last_raw;
    btn -> last_click_time = 0;
    btn -> press_time = 0;
    btn -> waiting_clicks = 0;
    btn -> held = 0;
}

static inline uint8_t buttonGetClicks(Button *btn){
    uint8_t clicks = btn -> click_ready;
    btn -> click_ready = 0;
    return clicks;
}

static inline uint8_t isClicked(Button *btn){
    return buttonGetClicks(btn) == 1;
}

static inline uint8_t isDoubleClicked(Button *btn){
    return buttonGetClicks(btn) == 2;
}

static inline uint8_t isPressed(Button *btn) {
    return btn -> stable == PRESSED;
}

static inline uint8_t isHeldImpl(Button *btn, uint32_t now, uint32_t hold_time_ms) {
    if ((btn -> stable == PRESSED) && ((uint32_t)(now - btn -> press_time) >= hold_time_ms)){
        btn -> held = 1;
        return 1;
    }
    return 0;
}

static inline void tickButton(Button *btn, uint32_t now){
    uint8_t raw = getRawButtonState(btn);
    if (raw != btn -> last_raw) {
        btn -> last_time = now;
        btn -> last_raw = raw;
    }
    if ((uint32_t)(now - btn -> last_time) >= DEBOUNCE_TIME_MS){
        if (btn -> stable != raw){
            if (btn -> stable == PRESSED && raw == RELEASED){
                if (!btn -> held){
                    btn -> click_count++;
                    btn -> last_click_time = now;
                    btn->waiting_clicks = 1;
                }
            } else if (btn -> stable == RELEASED && raw == PRESSED){
                btn -> press_time = now;
                btn -> held = 0;
            }
            btn -> stable = raw;
        }
    }

    if (btn->waiting_clicks && (uint32_t)(now - btn->last_click_time) >= CLICKS_TIME_MS) {
        btn->click_ready = btn->click_count;
        btn->click_count = 0;
        btn->waiting_clicks = 0;
    }
}

#endif
