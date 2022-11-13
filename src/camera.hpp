#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <glm/gtx/string_cast.hpp>

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    ZERO,
    FORE,
    BACK,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    WORLDFORE,
    WORLDBACK,
    WORLDUP,
    WORLDDOWN
};
// maximum speed
const float C = 32.0f; 

// Default camera values
const float YAW           =  90.0f;
const float PITCH         =   0.0f;
// const float SPEED         =  2.5f;
const float ACCELLERATION =   2.0f;
const float DRAG          =   2.0f;
const float SENSITIVITY   =   0.1f;
const float ZOOM          =  45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Direction;
    glm::vec3 Velocity;
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldFront;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementAccelleration;
    float MovementDrag; // both of these are the rest frame forces / rest frame masses
    float MovementC;
    // float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    int decelerate = 1;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, 1.0f)), Velocity(glm::vec3(0.0f, 0.0f, 0.0f)), MovementAccelleration(ACCELLERATION), MovementDrag(DRAG), MovementC(C), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, 1.0f)), Velocity(glm::vec3(0.0f, 0.0f, 0.0f)), MovementAccelleration(ACCELLERATION), MovementDrag(DRAG), MovementC(C), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    
    // returns the lorentz boost from the camera frame to the world frame
    // Note that the time axis is the the LAST slot, and that g has signature ---+ 
    glm::mat4 GetLorentzBoost()
    {
        glm::vec3 v = Velocity;  // maybe we want the opposite transfrom?
        float v2 = glm::dot(v, v);
        float g = glm::inversesqrt(1.0f - v2/glm::dot(MovementC, MovementC));
        float gm1ov2 = (g - 1.0f)/v2;
        if (glm::isnan(gm1ov2))
            gm1ov2 = 0;
        // constructed column major
        return glm::mat4(glm::vec4(1.0f + gm1ov2*v.x*v.x, gm1ov2*v.x*v.y, gm1ov2*v.x*v.z, -g*v.x/MovementC),
                         glm::vec4(gm1ov2*v.y*v.x, 1.0f + gm1ov2*v.y*v.y, gm1ov2*v.y*v.z, -g*v.y/MovementC),
                         glm::vec4(gm1ov2*v.z*v.x, gm1ov2*v.z*v.y, 1.0f + gm1ov2*v.z*v.z, -g*v.z/MovementC),
                         glm::vec4(-g*v.x/MovementC, -g*v.y/MovementC, -g*v.z/MovementC, g));
    }


    float getGamma()
    {
        return glm::inversesqrt(1.0f - glm::dot(Velocity, Velocity)/glm::dot(MovementC, MovementC));
    }


    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction)
    {
        // direction should be normalised and set to zero in Update
        switch (direction){
            case FORE:
                Direction += Front; break;
            case BACK:
                Direction -= Front; break;
            case LEFT:
                Direction -= Right; break;
            case RIGHT:
                Direction += Right; break;
            case UP:
                Direction += Up; break;
            case DOWN:
                Direction -= Up; break;
            case WORLDUP:
                Direction += WorldUp; break;
            case WORLDDOWN:
                Direction -= WorldUp; break;
            case WORLDFORE:
                Direction += WorldFront; break;
            case WORLDBACK:
                Direction -= WorldFront; break;
            case ZERO:
                decelerate = 0; break;
        }
    }
    

    // this must be called every frame to update positions and velocities
    void Update(float deltaTime)
    {
        // the way update is done will tend to accumlate errors
        // this isn't really a problem as all that matters is that things feel ok
        
        // we use 1.01f instead of 1.0, to compensate for the fact that we should be calucalating g _after_ updating velocity. This is innefficient in practice, so we cheat. The casual observer won't notice anyway.
        float oneOverg = glm::sqrt(1.0f - glm::dot(Velocity, Velocity)/glm::dot(MovementC, MovementC));
        // deltaTime is world time - i.e. in the rest frame of the scene
        // float dTau = deltaTime/g;
        
        // forceOm = Force/RestMass (in rest frame)
        glm::vec3 forceOm;
        float D2 = glm::dot(Direction, Direction);
        if (D2 != 0.0f)
        {
            forceOm = Direction/glm::sqrt(D2)*MovementAccelleration;
            Direction = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        else if (decelerate)
        {   forceOm = -MovementDrag*Velocity;    }
        else
        {    forceOm = glm::vec3(0.0f, 0.0f, 0.0f);   }
        decelerate = 1; // flag on wether or not to decellerate the player
        if (glm::dot(forceOm, forceOm) >= 0.0000001f)  // 0.0001f squared
        {
            glm::vec3 acc = (forceOm - glm::dot(forceOm, Velocity)*Velocity/glm::dot(MovementC, MovementC))*oneOverg;
            Velocity += acc * deltaTime;
        } else if ((glm::dot(Velocity, Velocity) <= 0.0000001f) || glm::isnan(glm::dot(Velocity, Velocity))  )
                Velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        // std::cout << "v = " << glm::to_string(Velocity) << std::endl;
        
        Position += Velocity * deltaTime;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f; 
    }




private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up    = glm::normalize(glm::cross(Right, Front));
        
        front.y = 0.0f;
        WorldFront = glm::normalize(front);
    }
};
#endif
