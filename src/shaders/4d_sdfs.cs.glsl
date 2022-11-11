#version 430
layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) restrict writeonly uniform image2D img;
// layout(rgba32f, binding = 1) uniform image2D img_output;

// layout(binding=0) uniform sampler2D img_input;
uniform mat4 view;
uniform mat4 B;  // Lorentz boost matrix from the camera's frame
uniform vec2 imsize;
uniform vec4 vel;
// uniform float tau; // camera time 
uniform float time; // world time

float sdf(in vec3 pin){
    vec3 p = mod(pin+vec3(1.0), vec3(2.0))-vec3(1.0);
    // if (pin.x > 0.0){
    // const vec2 torus_dim = vec2(0.5, 0.25);
    // vec2 q = vec2(length(p.xz)-torus_dim.x,p.y);
    // return length(q)-torus_dim.y;
    // }

    const vec3 b = vec3(0.5,0.5,0.5);
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
    
}


// https://iquilezles.org/articles/normalsSDF
// It's nothing very exciting; some well tuned values and tetrahedral sampling
vec3 calcNormal(in vec3 pos ){
    vec2 e = vec2(1.0,-1.0)*0.5773;
    const float eps = 0.0005;
    return normalize( e.xyy*sdf( pos + e.xyy*eps ) + 
					  e.yyx*sdf( pos + e.yyx*eps ) + 
					  e.yxy*sdf( pos + e.yxy*eps ) + 
					  e.xxx*sdf( pos + e.xxx*eps ) );
}


void main() {
    uint gid = (gl_WorkGroupID.z*gl_NumWorkGroups.y*gl_NumWorkGroups.x
              + gl_WorkGroupID.y*gl_NumWorkGroups.x
              + gl_WorkGroupID.x);
    gid *= gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;
    gid += gl_LocalInvocationIndex; // I think order matters to avoid cache misses

    vec2 pixel_coords = vec2(gid/int(imsize.y), gid%int(imsize.y));
    // add check for end of array
    
    vec2 p = (-imsize.xy + 2.0*pixel_coords)/imsize.y;

    // create view ray
    // vec3 rd = normalize( p.x*uu + p.y*vv + 1.5*ww );
    vec3 ro = view[3].xyz;
    vec3 rd = mat3(view)*vec3(p, -1.5); // mat3() will take the top lefthand corner

    // we will have to move the world, and keep the camera still
    // Work out how to calculate the correction new direction
    vec4 kprime = B*vec4(rd, 1.0); // 1.0rad s-1 / c ???

    // shift the world to make it look like the rays are moving
    rd = normalize(kprime.xyz + B[3].xyz);
    
    // kprime.w will give the change in colour frequency.
    // Use a small 3d texture (generated in code?) to do the conversion from velocity shifted color to rgb
    // CIE 1931 colour space
    // GET YOUR COLOURS HERE!!! https://www.fourmilab.ch/documents/specrend/


   // raymarch
   const float tmax = 13.0;
   float t = 0.0;
   for( int i=0; i<256; i++ )
   {
       vec3 pos = ro + t*rd;
       float h = sdf(pos);
       if( abs(h)<0.0001 || t>tmax ) break;
       t += h*0.95;
   }
   
   
   // shading/lighting	
   vec3 col = vec3(0.0);
   if( t<tmax )
   {
       vec3 pos = ro + t*rd;
       vec3 nor = calcNormal(pos);
       float dif = clamp( dot(nor,vec3(0.7,0.6,0.4)), 0.0, 1.0 );
       float amb = 0.5 + 0.5*dot(nor,vec3(0.0,0.8,0.6));
       col = vec3(0.2,0.3,0.4)*amb + vec3(0.8,0.7,0.5)*dif;
   } 
    
    // col = t>=10 ? vec3(0.0) : vec3(1.0);
    imageStore(img, ivec2(pixel_coords), vec4(sqrt(col), 1.0));
}




// vec3 hsv2rgb(vec3 c){
//     vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
//     vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
//     return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
// }
