cmake_minimum_required(VERSION 3.10)

# Явно указываем что это embedded система
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

project(stm32_template C ASM)

# Компиляторы
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(OBJCOPY arm-none-eabi-objcopy)

# Полностью отключаем стандартные флаги CMake
set(CMAKE_C_STANDARD_LIBRARIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES "")

# Флаги компилятора
set(CMAKE_C_FLAGS "-mcpu=cortex-m3 -mthumb -std=gnu99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections -ffunction-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSTM32F103C8 -DSTM32F10X_MD")

# Флаги ассемблера
set(CMAKE_ASM_FLAGS "-mcpu=cortex-m3 -mthumb")

# Флаги линковки - ПРОСТОЙ формат
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -nostartfiles")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=stm32_template.map")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${CMAKE_SOURCE_DIR}/stm32f103c8.ld")

# Переопределяем команду линковки чтобы убрать лишние флаги
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> -Wl,--start-group <LINK_LIBRARIES> -Wl,--end-group")

# Директории
include_directories(cmsis stm32)

# Исходные файлы
set(SOURCES
    main.c
    stm32/startup/startup_stm32f10x_md.c
    stm32/system_stm32f10x.c
    syscalls/syscalls.c
)

# Создаем исполняемый файл
add_executable(stm32_template ${SOURCES})

# Создаем bin файл
add_custom_command(TARGET stm32_template POST_BUILD
    COMMAND ${OBJCOPY} -O binary $<TARGET_FILE:stm32_template> stm32_template.bin
    COMMENT "Creating stm32_template.bin"
)

# Создаем hex файл
add_custom_command(TARGET stm32_template POST_BUILD
    COMMAND ${OBJCOPY} -O ihex $<TARGET_FILE:stm32_template> stm32_template.hex
    COMMENT "Creating stm32_template.hex"
)