## A Relativistic Jet-Pack
You have strapped yourself into a super-flux-quantum-thingummy™ jet-pack, capable of accelerations of a ludicrous 4 million g without pulverising it's occupant. As you hurtle through space, you are grateful that your super-flux-quantum-thingummy™ jet-pack also places you in ultra-electro-magnetic-phase-space™, and thus incapable of colliding with the rather lackluster polygons occupying this corner of the universe, otherwise polygons would quickly gain the appearance of Swiss cheese with super-flux-quantum-thingummy™ jet-pack shaped holes.
To reduce motion sickness the super-flux-quantum-thingummy™ jet-pack is equipped with inertial damping, and will start to reduce your velocity as soon as you stop accelerating.


Alternatively, you float serenely though a rather curious universe where the speed of light has been lowered to 20ms-1, with an instantaneous rest frame acceleration of 4ms-2.
When not pressing a direction key, you decelerate at -1ms-2.


# Key Bindings
[W]          - accelerate camera forwards
[S]          - accelerate camera backwards
[A]          - accelerate camera left
[D]          - accelerate camera right
[space]      - accelerate camera up
[left_shift] - accelerate camera down
[E]          - don't null camera velocity
[F12]        - make full screen
[escape]     - exit full screen
[ctrl]+[W]   - close application
[ctrl]+[Q]   - close application


# Manifest
This code shows a view of a world when travelling at relativistic speed.
This is done by ray tracing signed distance functions (SDFs), generalised to 4-space.
Details of SDFs can be found on [Inigo Quilez's Website](https://iquilezles.org/articles/).

This project was started after noting that the visual aids used in lectures on special relativity were either static images or terribly pixelated. I have not looked, but there are likely better inplementations of special relativity in demos around the internet, but if I tried to do things that no one had done before I would never get started.

Requires GLFW 3.3.6 (or greater) dynamically linked libraries (not included), and a graphics device that supports OpenGL 4.3 or higher. (This will include most graphics cards made after 2017).
GLFW3 is available from your favourite Linux package manager, and Windows and macOS binaries are available from the [GLFW website](https://www.glfw.org/download.html).

Compilation can be performed the following, or an equivalent command, from the root directory
g++ -I./include src/4d_sdf.cpp src/shader_utils.cpp src/glad.c -lglfw -O3 -o 4d_sdf

The code currently assumes that the shader files are found in ./src/shaders/ relative to the execution directory, so moving the executable is not recommended.


# Licence 
Some code for this project was taken from [Joey de Vries' learn OpenGL tutorial](https://github.com/JoeyDeVries/LearnOpenGL), and [Inigo Quilez's website](https://iquilezles.org/) (under the CC BY-NC 4.0 and MIT licences respectively).
This code is released under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/)

