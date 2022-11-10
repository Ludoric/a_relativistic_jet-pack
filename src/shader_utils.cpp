#include "shader_utils.hpp"

// The terrible Nvidia card in my laptop is worse than the intel soc
//      so I have disabeled this section
#if 0
// Equivelent to GLFW_USE_HYBRID_HPG, but for the dll version
// (use the most powerful graphics processor on window systems with multiple)
#ifndef _WINDEF_
typedef unsigned long DWORD;
extern "C"{
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif
#endif



int get_file_contents(const char * filename, GLchar * strdata, int maxlen){
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in){
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        if(maxlen < (int)contents.size()){
            throw(errno);
        }
        contents.copy(strdata, (std::size_t)(maxlen-1)*sizeof(GLchar));
        if (contents.size()<(std::size_t)maxlen-1){
            strdata[contents.size()] = '\0';
        }//silly way to tack on the null byte
        // memcpy(strdata, contents.c_str(), sizeof(GLchar)*std::max((std::size_t)maxlen, contents.size()));
        return(maxlen - (int)contents.size()); // return remaining space in buffer
    }
    throw(errno);
}

void print_shader_info_log(GLuint shader){
    const int max_length    = 4096;
    int actual_length = 0;
    char slog[max_length];
    glGetShaderInfoLog(shader, max_length, &actual_length, slog);
    std::cerr << "shader info log for GL index " << shader << std::endl;
    std::cerr <<  slog << std::endl;
}

void print_program_info_log(GLuint program){
    const int max_length    = 4096;
    int actual_length = 0;
    char plog[max_length];
    glGetProgramInfoLog(program, max_length, &actual_length, plog);
    std::cerr << "program info log for GL index " << program << std::endl;
    std::cerr <<  plog << std::endl;
}

bool check_shader_errors(GLuint shader){
    GLint params = -1;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &params);
    if (GL_TRUE != params) {
        std::cerr << "ERROR: shader " << shader << " did not compile" << std::endl;
        print_shader_info_log(shader);
        return false;
    }
    return true;
}

bool check_program_errors(GLuint program){
    GLint params = -1;
    glGetProgramiv(program, GL_LINK_STATUS, &params);
    if (GL_TRUE != params) {
        std::cerr << "ERROR: program " << program << " did not compile" << std::endl;
        print_program_info_log(program);
        return false;
    }
    return true;
}

GLuint create_shader(const char * filename, GLuint shadertype){
    GLuint shader = glCreateShader(shadertype);
    GLchar *shader_str = (GLchar *)malloc(sizeof(GLchar)*8192);
    try{
        int remaining = get_file_contents(filename, shader_str, 8192);
        if (remaining < 1000){
            std::cout << filename << std::endl;
            std::cout << "\tRemaining space in buffer " << remaining << std::endl;
        }
    }catch(int e){
        std::cerr << "FAILED TO READ FILE CORRECTLY: " << e << std::endl;
    }
    glShaderSource(shader, 1, &shader_str, NULL);
    glCompileShader(shader);
    check_shader_errors(shader);
    free(shader_str);
    return shader;
}
