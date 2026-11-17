#!/bin/bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake --build build && ./build/progetto_computer_grafica
