#include <stdint.h>
#include <stm32f10x.h>

// Определение пинов для кнопок
#define BTN_UP_PIN    15  // PC15 - уменьшение частоты
#define BTN_DOWN_PIN  14  // PC14 - увеличение частоты  
#define BTN_POWER_PIN 13  // PC13 - светодиод

// Константы для экспоненты
#define EXP_MIN   (-6)
#define EXP_MAX   ( 6)

static volatile int8_t exp_power = 0;

// Базовые значения таймера
#define PSC_BASE 999
#define ARR_BASE 35999


void delay(uint32_t count)
{
    for(uint32_t i = 0; i < count; i++){ __NOP(); }
}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        // Сбрасываем флаг прерывания
        TIM2->SR &= ~TIM_SR_UIF;
        
        // Переключаем состояние светодиода PC13
        GPIOC->ODR ^= GPIO_ODR_ODR13;
    }
}

void timer_init(void)
{
    // Тактирование TIM2
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    
    // Сброс TIM2
    RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;
    
    // Настройка предделителя и предельного значения
    TIM2->PSC = PSC_BASE;
    TIM2->ARR = ARR_BASE;
    
    // Включение прерывания по обновлению
    TIM2->DIER |= TIM_DIER_UIE;
    
    // Настройка NVIC
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    NVIC_EnableIRQ(TIM2_IRQn);
    
    // Запуск таймера
    TIM2->CR1 |= TIM_CR1_CEN;
}

void gpio_init(void)
{
    // Включаем тактирование порта C
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    
    // Настраиваем PC13 как выход для светодиода
    GPIOC->CRH = (GPIOC->CRH & ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13)) | GPIO_CRH_MODE13_0;
    GPIOC->ODR |= GPIO_ODR_ODR13; // Изначально выключен
    
    // Настраиваем PC14 и PC15 как входы с подтяжкой к питанию (кнопки)
    GPIOC->CRH = (GPIOC->CRH & ~(GPIO_CRH_CNF14 | GPIO_CRH_MODE14)) | GPIO_CRH_CNF14_1;
    GPIOC->CRH = (GPIOC->CRH & ~(GPIO_CRH_CNF15 | GPIO_CRH_MODE15)) | GPIO_CRH_CNF15_1;
    
    // Включаем подтяжку к питанию для кнопок
    GPIOC->ODR |= (GPIO_ODR_ODR14 | GPIO_ODR_ODR15);
}

int main(void)
{

    gpio_init();
    timer_init();
    
    // Переменные для обработки кнопок 
    uint8_t btn_up_prev = 0, btn_down_prev = 0;
    uint8_t btn_up_current = 0, btn_down_current = 0;
    
    while (1)
    {
        // Чтение текущих состояний кнопок
        btn_up_current = (GPIOC->IDR & (1U << BTN_UP_PIN)) ? 1 : 0;     // PC15 - уменьшение частоты
        btn_down_current = (GPIOC->IDR & (1U << BTN_DOWN_PIN)) ? 1 : 0; // PC14 - увеличение частоты
        
        // Обработка кнопки PC14 (увеличение частоты)
        if (btn_down_current && !btn_down_prev) {
            delay(1000); // Антидребезг
            
            if (btn_down_current) {
                // Проверка границ
                if (exp_power < EXP_MAX) {
                    exp_power++;
                    
                    // Уменьшение PSC (увеличение частоты)
                    if (TIM2->PSC > 0) {
                        TIM2->PSC = TIM2->PSC >> 1;
                    }
                }
                
                // Ждём отпускания кнопки
                while ((GPIOC->IDR & (1U << BTN_DOWN_PIN)) == 0) {
                    __NOP();
                }
                delay(1000);
            }
        }
        
        // Обработка кнопки PC15 (уменьшение частоты)
        if (btn_up_current && !btn_up_prev) {
            delay(1000); // Антидребезг
            
            if (btn_up_current) {
                // Проверка границ
                if (exp_power > EXP_MIN) {
                    exp_power--;
                    
                    // Увеличение PSC (уменьшение частоты)
                    if (TIM2->PSC <= (0xFFFF / 2)) {
                        TIM2->PSC = TIM2->PSC << 1;
                    } else {
                        exp_power = EXP_MIN;
                        TIM2->PSC = 0xFFFF;
                    }
                }
                
                // Ждём отпускания кнопки
                while ((GPIOC->IDR & (1U << BTN_UP_PIN)) == 0) {
                    __NOP();
                }
                delay(1000);
            }
        }
        
        // Сохраняем предыдущие состояния кнопок
        btn_up_prev = btn_up_current;
        btn_down_prev = btn_down_current;
    }
}