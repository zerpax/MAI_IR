cmake_minimum_required(VERSION 3.27)
project(tokenize LANGUAGES CXX)

find_package(Python REQUIRED COMPONENTS Interpreter Development)

execute_process(
    COMMAND "${Python_EXECUTABLE}" -m pybind11 --cmakedir
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(tokenizer src/tokenize.cpp)
pybind11_add_module(stemmer src/stemming.cpp)
pybind11_add_module(zipf src/zipf.cpp)
pybind11_add_module(index src/index.cpp)

install(TARGETS tokenizer stemmer zipf index DESTINATION lib/)
