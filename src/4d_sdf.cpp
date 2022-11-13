#include "shader_utils.hpp"
#include "camera.hpp"
#include <cstdlib>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/norm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


// Wouldn't it be nice to specify the gpu used?


#define RANF(min, max) ((float)(std::rand())/RAND_MAX)*(max-min)+min


void GLAPIENTRY MessageCallback( GLenum source, GLenum type, GLuint id,
        GLenum severity, GLsizei length, const GLchar* message,
        const void* userParam);
void errorCallback_glfw(int code, const char * message);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

double mouse_y_prev = 0, mouse_x_prev = 0;
GLuint imgWidth = 800, imgHeight = 800;
Camera cam;


struct Window_info{int width, height, xpos, ypos; } window_info = {0, 0, 0, 0};


int main(){
    std::cout << "Starting GLFW context, OpenGL 4.3" << std::endl;
    // Init GLFW
    glfwSetErrorCallback(errorCallback_glfw);
    glfwInit();
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // compute shaders added in 4.3
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow* window = glfwCreateWindow(imgHeight, imgWidth, "A Relativistic Jet-Pack", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (window == NULL){
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)){
        std::cerr << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);


    // Define the viewport dimensions - we set the window size, not the framebuffer size earlier
    // glfwGetFramebufferSize(window, (int*)(&imgWidth), (int*)(&imgHeight));
    glViewport(0, 0, imgWidth, imgHeight);

    // send stuff to the user so they know somethings happening
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version  = glGetString(GL_VERSION);
    std::cout << "  Renderer: " << renderer << std::endl;
    std::cout << "  OpenGL Version: " << version << std::endl;

    // initialize the particle position


    // generate the shaders
    // Compute Program
    GLuint compute_shader = create_shader(
        (char const *)"./src/shaders/4d_sdfs.cs.glsl",
        GL_COMPUTE_SHADER
    );

    GLuint compute_program = glCreateProgram();
    glAttachShader(compute_program, compute_shader);
    glLinkProgram(compute_program);
    check_program_errors(compute_program);
    glDetachShader(compute_program, compute_shader); // clean up after linking

    glDeleteShader(compute_shader);


    // Display Program
    GLuint vert_shader = create_shader(
        (char const *)"src/shaders/vert_shader.vs.glsl", GL_VERTEX_SHADER
   );

    GLuint frag_shader = create_shader(
        (char const *)"src/shaders/frag_shader.fs.glsl", GL_FRAGMENT_SHADER
   );

    GLuint display_program = glCreateProgram();
    glAttachShader(display_program, vert_shader);
    glAttachShader(display_program, frag_shader);
    glLinkProgram(display_program);
    check_program_errors(display_program);
    glDetachShader(display_program, vert_shader);
    glDetachShader(display_program, frag_shader);

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);


    //========================================================================
    // setup the data structures to pass to the compute shader and render shader



     // texture handle and dimensions
    GLuint tex_pattern [] = {0};
    { // create the texture
        glGenTextures(1, tex_pattern);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_pattern[0] );
        // linear allows us to scale the window up retaining reasonable quality
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // same internal format as compute shader input

        // create the inital texture!!!!
        glActiveTexture(GL_TEXTURE0);
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, imgWidth, imgHeight, 0, GL_RGBA, GL_FLOAT, 0);
    }

    GLuint quad_vao = 0;
    {
    GLuint vao = 0, vbo = 0;
    float verts[] = { -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    glGenBuffers( 1, &vbo );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBufferData( GL_ARRAY_BUFFER, 16 * sizeof( float ), verts, GL_STATIC_DRAW );
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    glEnableVertexAttribArray( 0 );
    GLintptr stride = 4 * sizeof( float );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, stride, NULL );
    glEnableVertexAttribArray( 1 );
    GLintptr offset = 2 * sizeof( float );
    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)offset );
    quad_vao = vao;
    }

    // lambda2RGB texture

    GLuint tex_lambda2RGB;
    {
        glGenTextures(1, &tex_lambda2RGB);
        // glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_lambda2RGB);
        // linear allows us to scale the window up retaining reasonable quality
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        // same internal format as compute shader input

        // create the inital texture!!!!
        int width, height, nrChannels;
        unsigned char *data = stbi_load("src/lambda2RGB.png", &width, &height, &nrChannels, 0);
        if (data)
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        else
            std::cout << "FAILED TO LOAD IMAGE!!!" << std::endl;
        stbi_image_free(data);
    }
    //========================================================================

    // now just setup and run the loop

    // std::cout << "Start Binding 3D Textures" << std::endl;

    // std::cout << "Finished Binding 3D Textures" << std::endl;

    // glClear(GL_COLOR_BUFFER_BIT);
    glm::mat4 inverse_view;
    glm::mat4 lorentz_boost;
    

    
    glUseProgram(compute_program);
    // glActiveTexture(GL_TEXTURE1);
    // glBindTexture(GL_TEXTURE_2D, tex_lambda2RGB);
    glUniform1i(glGetUniformLocation(compute_program, "lambda2RGB"), 1);
    GLuint timeloc = glGetUniformLocation(compute_program, "time"); // the world's time
    // GLuint tauloc = glGetUniformLocation(compute_program, "tau"); // the camera's time
    GLuint viewloc = glGetUniformLocation(compute_program, "view");
    GLuint boostloc = glGetUniformLocation(compute_program, "B");
    // GLuint velloc = glGetUniformLocation(compute_program, "vel");
    GLuint imsizeloc = glGetUniformLocation(compute_program, "imsize");
    
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, tex_pattern[0]);
    glBindImageTexture( 0, tex_pattern[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F );
    glUseProgram(display_program);
    // glBindImageTexture( 0, tex_pattern[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F );

    
    cam = Camera(glm::vec3(0.0f, -1.0f, 0.0f));

    double print_timecounter = glfwGetTime();
    double last_print_Time = print_timecounter;
    double currentTime;
    int numFrames = 0;
    int totalFrameCounter = 0;

    glfwSetTime(0.0);
    double worldFrameTime = 0;
    double lastFrameTime = 0;

    while (!glfwWindowShouldClose(window)){ // drawing loop
        currentTime = glfwGetTime();
        totalFrameCounter++;
        numFrames++;
        if(currentTime >= print_timecounter){
            std::cout << numFrames << "fps  =  "
                << (currentTime-last_print_Time)*1000.0/((double)numFrames)
                << "mspf\tBeta = "
                << glm::l2Norm(cam.Velocity)/cam.MovementC << std::endl;
            numFrames = 0;
            last_print_Time = currentTime;
            print_timecounter += 1.0;
        }


        inverse_view = glm::inverse(cam.GetViewMatrix());
        lorentz_boost = cam.GetLorentzBoost();

        glUseProgram(compute_program); // gl shader commands refer to the compute shader
        // glUniform1f(tauloc, float(currentTime)); 
        glUniform1f(timeloc, float(worldFrameTime)); 
        glUniform2f(imsizeloc, float(imgWidth), float(imgHeight)); 
        // glUniform4fv(velloc, 1, glm::value_ptr(glm::vec4(cam.Velocity, cam.MovementC))); // send matrix to shader
        glUniformMatrix4fv(viewloc, 1, GL_FALSE, glm::value_ptr(inverse_view)); // send matrix to shader
        glUniformMatrix4fv(boostloc, 1, GL_FALSE, glm::value_ptr(lorentz_boost)); // send matrix to shader
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_pattern[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_lambda2RGB);
        glDispatchCompute(imgWidth, imgHeight, 1);

        // update paricle positions

        //ensure the compute is finished before starting to draw to the screen
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        // glFinish();
        // glFlush();

        glClear(GL_COLOR_BUFFER_BIT );
        glUseProgram(display_program); // commands now refer to the display shaders
        // glClear(GL_COLOR_BUFFER_BIT); // clear the screen
        // //glDrawArrays(GL_POINTS, 0, NUM_PARTICLES); // actually draw the dots
        glBindVertexArray(quad_vao );
        glActiveTexture(GL_TEXTURE0 );
        glBindTexture(GL_TEXTURE_2D, tex_pattern[0] );
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4 );

        // Swap the screen buffers
        glfwSwapBuffers(window); // no screen tearing please
        glfwPollEvents(); // get window close events + keyboard input
        processInput(window);
        cam.Update((float)(currentTime-lastFrameTime));
        worldFrameTime += (currentTime-lastFrameTime)/((double)cam.getGamma());
        lastFrameTime = currentTime;


    }

    // I think I don't need glDeleteBuffers(1, &velSSbo); etc.
    // glDeleteBuffers(1, &velSSbo);
    // glDeleteBuffers(1, &posSSbo);
    // glDeleteTextures(1, &tex_electric_loc);
    // glDeleteTextures(1, &tex_magnetic_loc);

    // Terminates GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();

    return 0;
}


void processInput(GLFWwindow *window){
    // if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
    //     if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
    //
    //     }
    // }
    //     glfwSetWindowShouldClose(window, true);
    // totalLoopsPerFrame = (glfwGetKey(window, GLFW_KEY_F)==GLFW_PRESS) ? 10 : 1;
    // dTime = (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) ? -0.001f : 0.001f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cam.ProcessKeyboard(WORLDFORE);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cam.ProcessKeyboard(WORLDBACK);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cam.ProcessKeyboard(LEFT);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cam.ProcessKeyboard(RIGHT);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cam.ProcessKeyboard(WORLDDOWN);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cam.ProcessKeyboard(WORLDUP);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cam.ProcessKeyboard(ZERO);
}

// WOOOHHH!!! ERRORS????
void GLAPIENTRY MessageCallback( GLenum source, GLenum type, GLuint id,
        GLenum severity, GLsizei length, const GLchar* message,
        const void* userParam){
    if (type == GL_DEBUG_TYPE_ERROR)  // don't print the warnings
    fprintf(stderr,
            "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message:\n\t%s\n",
           (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
    // if (severity ==0x9146) glfwSetWindowShouldClose(window, GL_TRUE);
}

void errorCallback_glfw(int code, const char * message){
    std::cerr << "GLFW ERROR: " << code << "Message: " << std::endl;
    std::cerr << "\t" << message << std::endl;
}

int tryMakeWindowed(GLFWwindow* window){
    if (window_info.width&&window_info.height&&window_info.xpos&&window_info.ypos){
        glfwSetWindowMonitor(window, NULL, window_info.xpos, window_info.ypos,
                window_info.width, window_info.height, 0);
        window_info = {0,0,0,0};
        return 1;
    }
    return 0;
}
int tryMakeFullscreen(GLFWwindow* window){
    if (!(window_info.width&&window_info.height&&window_info.xpos&&window_info.ypos)){
        glfwGetWindowSize(window, &window_info.width, &window_info.height);
        glfwGetWindowPos(window, &window_info.xpos, &window_info.ypos);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        // std::cout << monitor << std::endl;
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        // std::cout << mode << std::endl;
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width,
            mode->height, mode->refreshRate);
        return 1;
    }
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    // std::cout << "Key Callback" << std::endl;
    if (action == GLFW_PRESS && (key == GLFW_KEY_W || key == GLFW_KEY_Q)
            && (mods&GLFW_MOD_CONTROL)){
        glfwSetWindowShouldClose(window, GL_TRUE);
    }else if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE){
        tryMakeWindowed(window);
    }else if (action == GLFW_PRESS && key == GLFW_KEY_F11){
        if (!tryMakeFullscreen(window)){
            tryMakeWindowed(window);
        }
    }
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    // std::cout << "Changed window size" << std::endl;
    glViewport(0, 0, width, height);
    imgWidth  = width;
    imgHeight = height;
    
    glActiveTexture(GL_TEXTURE0);
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, imgWidth, imgHeight, 0, GL_RGBA, GL_FLOAT, 0);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
    cam.ProcessMouseMovement((float)(xpos - mouse_x_prev), -(float)(ypos - mouse_y_prev));
    mouse_x_prev = xpos;
    mouse_y_prev = ypos;
}

