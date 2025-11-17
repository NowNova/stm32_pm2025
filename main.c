#include <stdint.h>
#include <stm32f10x.h>

// Определение пинов для кнопок
#define BTN_UP_PIN    0  // PA0 - уменьшение частоты
#define BTN_DOWN_PIN  2  // PA2 - увеличение частоты  
#define BTN_POWER_PIN 14 // PC14 - вкл/выкл

// Пины для SSD1306
#define CS_PIN   4  // PA4 - /CE (Chip Enable)
#define DC_PIN   3  // PA3 - DC (Data/Command)  
#define RST_PIN  6  // PA6 - /RST (Reset)

// ==================== SPI БИБЛИОТЕКА ====================

void SPI1_Init(void)
{
    // Включаем тактирование SPI1
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    
    // Настраиваем SPI1 пины: PA5 (SCK), PA7 (MOSI), PA6 (MISO)
    GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE5 | 
                    GPIO_CRL_CNF6 | GPIO_CRL_MODE6 |
                    GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
    GPIOA->CRL |= (GPIO_CRL_CNF5_1 | GPIO_CRL_MODE5_1 |  // PA5: SCK
                   GPIO_CRL_CNF6_1 | GPIO_CRL_MODE6_0 |  // PA6: MISO
                   GPIO_CRL_CNF7_1 | GPIO_CRL_MODE7_1);  // PA7: MOSI
    
    // Configure SPI1
    SPI1->CR1 = SPI_CR1_CPOL | SPI_CR1_CPHA |     // SPI Mode 3
                SPI_CR1_MSTR |                    // Master mode
                SPI_CR1_BR_0 | SPI_CR1_BR_1 |     // Baud rate divider fPCLK/32
                SPI_CR1_SSM | SPI_CR1_SSI |       // Software slave management
                SPI_CR1_SPE;                      // SPI enable
}

void SPI1_Write(uint8_t data)
{
    // Ждем, пока не освободится буфер передатчика
    while(!(SPI1->SR & SPI_SR_TXE));
    // Заполняем буфер передатчика
    SPI1->DR = data;
}

uint8_t SPI1_Read(void)
{
    // Ждем, пока данные не будут получены
    while(!(SPI1->SR & SPI_SR_RXNE));
    // Читаем данные из буфера приемника
    return SPI1->DR;
}

// ==================== SSD1306 ДРАЙВЕР ====================

void delay(uint32_t count)
{
    for(uint32_t i = 0; i < count; i++){ __NOP(); }
}

void SSD1306_Reset(void)
{
    GPIOA->ODR &= ~(1U << RST_PIN);  // RST=0
    delay(100000);                    // Задержка 10ms
    GPIOA->ODR |= (1U << RST_PIN);   // RST=1
    delay(100000);                    // Задержка 10ms
}

void SSD1306_Command(uint8_t cmd)
{
    GPIOA->ODR &= ~(1U << CS_PIN);   // CS=0
    GPIOA->ODR &= ~(1U << DC_PIN);   // DC=0 (режим команды)
    SPI1_Write(cmd);
    GPIOA->ODR |= (1U << CS_PIN);    // CS=1
}

void SSD1306_Data(uint8_t data)
{
    GPIOA->ODR &= ~(1U << CS_PIN);   // CS=0
    GPIOA->ODR |= (1U << DC_PIN);    // DC=1 (режим данных)
    SPI1_Write(data);
    GPIOA->ODR |= (1U << CS_PIN);    // CS=1
}

void SSD1306_Init(void)
{
    // Аппаратный сброс
    SSD1306_Reset();
    
    // Последовательность команд инициализации SSD1306
    SSD1306_Command(0xAE); // Display OFF
    SSD1306_Command(0x20); // Memory addressing mode
    SSD1306_Command(0x00); // Horizontal addressing mode
    SSD1306_Command(0xB0); // Set page start address
    SSD1306_Command(0xC8); // Set COM output scan direction
    SSD1306_Command(0x00); // Set low column address
    SSD1306_Command(0x10); // Set high column address
    SSD1306_Command(0x40); // Set start line address
    SSD1306_Command(0x81); // Set contrast control
    SSD1306_Command(0xFF); // Contrast value
    SSD1306_Command(0xA1); // Set segment re-map
    SSD1306_Command(0xA6); // Set normal display
    SSD1306_Command(0xA8); // Set multiplex ratio
    SSD1306_Command(0x3F); // Multiplex ratio value (1/64)
    SSD1306_Command(0xA4); // Output follows RAM content
    SSD1306_Command(0xD3); // Set display offset
    SSD1306_Command(0x00); // No offset
    SSD1306_Command(0xD5); // Set display clock divide ratio
    SSD1306_Command(0xF0); // Set divide ratio
    SSD1306_Command(0xD9); // Set pre-charge period
    SSD1306_Command(0x22); // Pre-charge period value
    SSD1306_Command(0xDA); // Set COM pins hardware configuration
    SSD1306_Command(0x12); // COM pins configuration
    SSD1306_Command(0xDB); // Set VCOMH deselect level
    SSD1306_Command(0x20); // VCOMH value
    SSD1306_Command(0x8D); // Set charge pump
    SSD1306_Command(0x14); // Enable charge pump
    SSD1306_Command(0xAF); // Display ON
}

void SSD1306_Clear(void)
{
    for(uint8_t page = 0; page < 8; page++) {
        SSD1306_Command(0xB0 + page);
        SSD1306_Command(0x00);
        SSD1306_Command(0x10);
        
        for(uint8_t x = 0; x < 128; x++) {
            SSD1306_Data(0x00);
        }
    }
}

// ==================== ПРОЦЕДУРЫ ШАХМАТНОЙ ДОСКИ ====================

void Display_ChessBoard(void)
{
    uint8_t x, page;
    
    for(page = 0; page < 8; page++) {
        SSD1306_Command(0xB0 + page);
        SSD1306_Command(0x00);
        SSD1306_Command(0x10);
        
        for(x = 0; x < 128; x++) {
            uint8_t pattern = 0;
            
            // Классическая шахматная доска 8x8
            uint8_t cell_x = x / 16;  // 8 клеток по горизонтали (128/16=8)
            uint8_t cell_y = page;    // 8 клеток по вертикали (64/8=8)
            
            if((cell_x + cell_y) % 2 == 0) {
                pattern = 0x00; // Белая клетка - все пиксели выключены
            } else {
                pattern = 0xFF; // Черная клетка - все пиксели включены
            }
            
            SSD1306_Data(pattern);
        }
    }
}

void Display_ChessBoardInverted(void)
{
    uint8_t x, page;
    
    for(page = 0; page < 8; page++) {
        SSD1306_Command(0xB0 + page);
        SSD1306_Command(0x00);
        SSD1306_Command(0x10);
        
        for(x = 0; x < 128; x++) {
            uint8_t pattern = 0;
            
            // Инвертированная шахматная доска 8x8
            uint8_t cell_x = x / 16;  // 8 клеток по горизонтали (128/16=8)
            uint8_t cell_y = page;    // 8 клеток по вертикали (64/8=8)
            
            if((cell_x + cell_y) % 2 == 0) {
                pattern = 0xFF; // Белая клетка становится черной
            } else {
                pattern = 0x00; // Черная клетка становится белой
            }
            
            SSD1306_Data(pattern);
        }
    }
}

// ==================== ОСНОВНАЯ ПРОГРАММА ====================

int main(void)
{
    // Включаем тактирование портов
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPAEN;
    
    // Инициализация SPI
    SPI1_Init();
    
    // Настраиваем PC13 как выход для светодиода
    GPIOC->CRH = GPIOC->CRH & ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13) | GPIO_CRH_MODE13_0;
    
    // Настраиваем PC14 как вход с подтяжкой к питанию (кнопка вкл/выкл)
    GPIOC->CRH = GPIOC->CRH & ~(GPIO_CRH_CNF14 | GPIO_CRH_MODE14) | GPIO_CRH_CNF14_1;
    GPIOC->BSRR = GPIO_BSRR_BS14;
    
    // Настраиваем PA0 и PA2 как входы с подтяжкой к земле (кнопки частоты)
    GPIOA->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_CNF2 | GPIO_CRL_MODE0 | GPIO_CRL_MODE2);
    GPIOA->CRL |= (GPIO_CRL_CNF0_1 | GPIO_CRL_CNF2_1);
    GPIOA->BRR = (1U << BTN_UP_PIN) | (1U << BTN_DOWN_PIN);
    
    // Настраиваем пины управления дисплеем как выходы: PA4 (CS), PA3 (DC), PA6 (RST)
    GPIOA->CRL &= ~(GPIO_CRL_CNF3 | GPIO_CRL_MODE3 | 
                    GPIO_CRL_CNF4 | GPIO_CRL_MODE4);
    GPIOA->CRL |= (GPIO_CRL_MODE3_0 | GPIO_CRL_MODE4_0);
    
    // Настраиваем пин RST (PA6) как выход
    GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
    GPIOA->CRL |= GPIO_CRL_MODE6_0;
    
    // Инициализация SSD1306
    GPIOA->ODR |= (1U << CS_PIN); // CS=1 (неактивен)
    SSD1306_Init();
    SSD1306_Clear();
    
    // Отображаем классическую шахматную доску при старте
    Display_ChessBoard();
    
    uint32_t i;
    uint8_t button_state = 0;
    uint8_t btn_up_prev = 0, btn_down_prev = 0;
    uint8_t btn_power_prev = 0;
    uint8_t display_mode = 0; // 0-классическая, 1-инвертированная
    
    // Базовая переменная задержки для частоты 1 Гц
    uint32_t current_delay = 250000;
    
    while(1)
    {
        // Проверяем нажатие кнопки PC14 (вкл/выкл мигания)
        uint8_t btn_power_current = (GPIOC->IDR & (1U << BTN_POWER_PIN)) ? 0 : 1;
        if (btn_power_current && !btn_power_prev) {
            button_state = ~button_state;
            for(i=0; i<8000; i++){ __NOP(); };
        }
        btn_power_prev = btn_power_current;
        
        // Чтение кнопок управления частотой
        uint8_t btn_up_current = (GPIOA->IDR & (1U << BTN_UP_PIN)) ? 1 : 0;    // PA0 - уменьшение
        uint8_t btn_down_current = (GPIOA->IDR & (1U << BTN_DOWN_PIN)) ? 1 : 0; // PA2 - увеличение
        
        // Обработка кнопки PA0 (уменьшение частоты - увеличение задержки + смена доски)
        if (btn_up_current && !btn_up_prev) {
            if (current_delay < 16000000) {
                current_delay <<= 1; // Умножаем задержку на 2
            }
            // Переключение между классической и инвертированной доской
            display_mode = (display_mode + 1) % 2;
            if(display_mode == 0) {
                Display_ChessBoard(); // Классическая доска
            } else {
                Display_ChessBoardInverted(); // Инвертированная доска
            }
            for(i=0; i<8000; i++){ __NOP(); };
        }
        
        // Обработка кнопки PA2 (увеличение частоты - уменьшение задержки)
        if (btn_down_current && !btn_down_prev) {
            if (current_delay > 3906) {
                current_delay >>= 1; // Делим задержку на 2
            }
            for(i=0; i<8000; i++){ __NOP(); };
        }
        
        // Сохраняем предыдущие состояния кнопок
        btn_up_prev = btn_up_current;
        btn_down_prev = btn_down_current;
        
        // Управление светодиодом
        if(button_state) {
            // Включен режим мигания
            GPIOC->ODR &= ~GPIO_ODR_ODR13; // LED ON
            for(i=0; i<current_delay; i++){ __NOP(); };
            GPIOC->ODR |= GPIO_ODR_ODR13;  // LED OFF
            for(i=0; i<current_delay; i++){ __NOP(); };
        } else {
            // Светодиод выключен
            GPIOC->ODR |= GPIO_ODR_ODR13;
        }
    }
}