#version 430
layout (location = 2) in vec2 st;
uniform sampler2D img;
out vec4 fc;

void main () {
    fc = texture(img, st);
}
