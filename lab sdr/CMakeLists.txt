cmake_minimum_required(VERSION 3.16)
project(PlutoSDR C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 11)

# Пути к заголовкам SoapySDR
include_directories(/usr/local/include)

# Исходники
set(MAIN_SOURCE_FILES main.c)

# Создание исполняемого файла
add_executable(main ${MAIN_SOURCE_FILES})

# Линковка с SoapySDR
target_link_libraries(main /usr/local/lib/libSoapySDR.so m pthread)