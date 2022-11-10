#! /bin/bash
g++ -I./include src/4d_sdf.cpp src/shader_utils.cpp src/glad.c -lglfw -O3 -o 4d_sdf
./4d_sdf
