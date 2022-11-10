#version 430
layout (location = 0) in vec2 vp;
layout (location = 1) in vec2 vt;
layout (location = 2) out vec2 st;

// uniform mat4 view;

void main () {
    st = vt;
    gl_Position = vec4(vp, 0.0, 1.0);
    // gl_Position = view * vec4(pos.xyz, 1.0);
}
