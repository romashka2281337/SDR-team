cmake_minimum_required(VERSION 3.16)
project(PlutoSDR C)

set(CMAKE_BUILD_TYPE Debug)

# Опция для включения или отключения установки зависимостей
option(INSTALL_DEPS "Установить зависимости" ON)
option(PLUTO_TIMESTAMP "Build pluto with timestamp" ON)

# RF frontend driver
find_package(SoapySDR REQUIRED)

# Исходники
set(MAIN_SOURCE_FILES
    main.c
)

# Исполняемый файл
add_executable(main ${MAIN_SOURCE_FILES})

# Пути к include для SoapySDR
target_include_directories(main PRIVATE ${SoapySDR_INCLUDE_DIRS})

# Линковка с библиотекой SoapySDR
target_link_libraries(main PRIVATE ${SoapySDR_LIBRARIES})