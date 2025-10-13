#include <stdint.h>
#include <stm32f10x.h>

// Определение пинов для кнопок джойстика
#define BTN_UP_PIN    0  // PA0 - кнопка "A" (вверх)
#define BTN_DOWN_PIN  2  // PA2 - кнопка "C" (вниз)

int main(void)
{
    // Включаем тактирование портов
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPAEN;
    
    // Настраиваем PC13 как выход для светодиода
    GPIOC->CRH = GPIOC->CRH & ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13) | GPIO_CRH_MODE13_0; // PC13 = output
    
    // Настраиваем PC14 как вход с подтяжкой к питанию (кнопка вкл/выкл)
    GPIOC->CRH = GPIOC->CRH & ~(GPIO_CRH_CNF14 | GPIO_CRH_MODE14) | GPIO_CRH_CNF14_1; // PC14 = Input with pull-up/pull-down
    GPIOC->BSRR = GPIO_BSRR_BS14; // Подтяжка к питанию для PC14
    
    // Настраиваем PA0 и PA2 как входы с подтяжкой к земле (кнопки частоты)
    GPIOA->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_CNF2 | GPIO_CRL_MODE0 | GPIO_CRL_MODE2);
    GPIOA->CRL |= (GPIO_CRL_CNF0_1 | GPIO_CRL_CNF2_1); // Input with pull-up/pull-down
    GPIOA->BRR = (1U << BTN_UP_PIN) | (1U << BTN_DOWN_PIN); // Pull-down
    
    uint32_t i;
    uint8_t button_state = 0;
    uint8_t btn_up_prev = 0, btn_down_prev = 0;
    
    // Таблица задержек для частот от 1/64 Гц до 64 Гц
    const uint32_t delay_table[] = {
        16000000,  // 1/64 Гц
        8000000,   // 1/32 Гц
        4000000,   // 1/16 Гц
        2000000,   // 1/8 Гц
        1000000,   // 1/4 Гц
        500000,    // 1/2 Гц
        250000,    // 1 Гц (начальная)
        125000,    // 2 Гц
        62500,     // 4 Гц
        31250,     // 8 Гц
        15625,     // 16 Гц
        7812,      // 32 Гц
        3906       // 64 Гц
    };
    
    uint8_t current_delay_index = 6; // Начальный индекс для 1 Гц
    uint32_t current_delay = delay_table[current_delay_index];
  
    while(1)
    {
        // Проверяем нажатие кнопки PC14 (вкл/выкл мигания)
        if(!(GPIOC->IDR & GPIO_IDR_IDR14)) { // Button is pressed
            button_state = ~button_state;
            for(i=0; i<300000; i++){ __NOP(); }; // Debouncing
            while(!(GPIOC->IDR & GPIO_IDR_IDR14)); // Wait for button release
            for(i=0; i<300000; i++){ __NOP(); }; // Debouncing after release
        }
        
        // Чтение кнопок управления частотой (PA0 и PA2)
        uint8_t btn_up_current = (GPIOA->IDR & (1U << BTN_UP_PIN)) ? 1 : 0;
        uint8_t btn_down_current = (GPIOA->IDR & (1U << BTN_DOWN_PIN)) ? 1 : 0;
        
        // Обработка кнопки "A" (увеличение частоты)
        if (btn_up_current && !btn_up_prev) {
            if (current_delay_index > 0) {
                current_delay_index--;
                current_delay = delay_table[current_delay_index];
            }
            for(i=0; i<100000; i++){ __NOP(); }; // Задержка антидребезга
        }
        
        // Обработка кнопки "C" (уменьшение частоты)
        if (btn_down_current && !btn_down_prev) {
            if (current_delay_index < (sizeof(delay_table)/sizeof(delay_table[0]) - 1)) {
                current_delay_index++;
                current_delay = delay_table[current_delay_index];
            }
            for(i=0; i<100000; i++){ __NOP(); }; // Задержка антидребезга
        }
        
        // Сохраняем предыдущие состояния кнопок частоты
        btn_up_prev = btn_up_current;
        btn_down_prev = btn_down_current;
        
        if(button_state) {
            // Включен режим мигания с текущей частотой
            GPIOC->ODR &= ~GPIO_ODR_ODR13; // LED ON
            for(i=0; i<current_delay; i++){ __NOP(); };
            GPIOC->ODR |= GPIO_ODR_ODR13;  // LED OFF
            for(i=0; i<current_delay; i++){ __NOP(); };
        } else {
            // Светодиод выключен
            GPIOC->ODR |= GPIO_ODR_ODR13; // LED OFF
        }
    }
}