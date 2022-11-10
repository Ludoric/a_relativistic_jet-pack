#pragma once

// GLAD
#include <glad/glad.h>

// GLFW
#define GLFW_DLL // may only be required by windows
#include <GLFW/glfw3.h>


#include <iostream>
#include <fstream>
#include <string>
#include <cerrno>
#include <cstring>

// Window dimensions
// extern GLuint WIDTH, HEIGHT;

int get_file_contents(const char * filename, GLchar * strdata, int maxlen);

void print_shader_info_log(GLuint shader);
void print_program_info_log(GLuint program);
bool check_shader_errors(GLuint shader);
bool check_program_errors(GLuint program);
GLuint create_quad_program(void);
GLuint create_shader(const char * filename, GLuint shadertype);
