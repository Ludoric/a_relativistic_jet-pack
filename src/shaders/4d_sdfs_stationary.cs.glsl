#version 430
layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) restrict writeonly uniform image2D img;

uniform sampler2D lambda2RGB;

uniform mat4 view;
uniform mat4 B;  // Lorentz boost matrix from the camera's frame
uniform vec2 imsize;
// uniform float tau; // camera time 
uniform float time; // world time


vec2 union_ab( vec2 a, vec2 b ){
	return (a.x<b.x) ? a : b;
}

vec2 map(in vec3 pin){
    // vec3 p = mod(pin+vec3(1.0), vec3(2.0))-vec3(1.0);
    const vec2 c = vec2(10.0, 2.0);
    vec3 p = vec3(mod(pin.x+0.5*c.x - 1.0, c.x)-0.5*c.x, pin.y-0.5, mod(pin.z+0.5*c.y,c.y)-0.5*c.y);
    
    const vec2 torus_dim = vec2(0.5, 0.25);
    vec2 q = vec2(length(p.xz)-torus_dim.x,p.y);
    return vec2(length(q)-torus_dim.y, 1.0);
}



// https://iquilezles.org/articles/checkerfiltering
float checkersGradBox( in vec2 p, in vec2 dpdx, in vec2 dpdy )
{
    // filter kernel
    vec2 w = abs(dpdx)+abs(dpdy) + 0.001;
    // analytical integral (box filter)
    vec2 i = 2.0*(abs(fract((p-0.5*w)*0.5)-0.5)-abs(fract((p+0.5*w)*0.5)-0.5))/w;
    // xor pattern
    return 0.5 - 0.5*i.x*i.y;                  
}


/* 
 * returns t (mulitple of rd), m
 */
vec2 raycast(in vec3 ro, in vec3 rd){
    vec2 res = vec2(-1.0, -1.0);// raymarch
    float tmin = 0.2;
    float tmax = 80.0;

    // raytrace floor plane
    float tp1 = (0.0-ro.y)/rd.y;
    if (tp1>0.0){
        tmax = min(tmax, tp1);
        res = vec2(tp1, 0.0);
    }

    if ((abs(ro.y) < 2.0) || (rd.y*ro.y<0)){
        float t = tmin;
        for (int i=0; i<192 && t<tmax; i++){
            vec2 h = map(ro + t*rd);
            if (abs(h.x) < (0.0001*t)){
                res = vec2(t, h.y);
                break;
            }
            t += h.x;
        }
    }
    return res;
}


// https://iquilezles.org/articles/normalsSDF
// It's nothing very exciting; some well tuned values and tetrahedral sampling
vec3 calcNormal(in vec3 pos ){
    vec2 e = vec2(1.0,-1.0)*0.5773;
    const float eps = 0.0005;
    return normalize(e.xyy*map(pos + e.xyy*eps).x + 
					 e.yyx*map(pos + e.yyx*eps).x + 
					 e.yxy*map(pos + e.yxy*eps).x + 
					 e.xxx*map(pos + e.xxx*eps).x);
}


/* 
 * rdx, rdy are solely used by the antialiasing of the checkerboard pattern
 */
vec3 render(in vec3 ro, in vec3 rd, in vec3 rdx, in vec3 rdy, in float wr){
    // background
    vec3 col = vec3(0.7, 0.7, 0.9) - max(rd.y,0.0)*0.3;
    
    // kprime.w will give the change in colour frequency.
    // I/omega^3 = invarient
    float intensity = 0.7/(wr*wr*wr);
    

    vec2 res = raycast(ro, rd);
    float t = res.x;
    float m =  res.y;
    
    if (m > -0.5){
        vec3 pos = ro + t*rd;
        vec3 nor = vec3(0.0, 1.0, 0.0); // plane normal

        if (m<=0.0){// it's on the plane
            // more magic from Inigo Quilez
            // project pixel footprint into the plane
            vec3 dpdx = ro.y*(rd/rd.y-rdx/rdx.y);
            vec3 dpdy = ro.y*(rd/rd.y-rdy/rdy.y);

            float f = checkersGradBox( 3.0*pos.xz, 3.0*dpdx.xz, 3.0*dpdy.xz);
            col = 0.15 + f*vec3(0.05);
            // ks = 0.4;
        }else{
            nor = calcNormal(pos);
            // Use a small 3d texture (generated in code?) to do the conversion from velocity shifted color to rgb
            // start with 550nm light (green)
            col = texture(lambda2RGB, vec2(((550.0*wr)-380.0)/(750.0-380.0), 0)).xyz;
            // vec3 c = texture(lambda2RGB, vec2(((550.0*kprime.w)-256.666)/616.666, 0)).xyz;
        }

        // shading/lighting	
        float dif = clamp( dot(nor,vec3(0.7,0.6,0.4)), 0.0, 1.0);
        float amb = 0.5 + 0.5*dot(nor,vec3(0.0,0.8,0.6));
        // col = vec3(0.2, 0.3, 0.4)*amb + vec3(0.8,0.7,0.5)*dif;  // vec3(0.2, 0.3, 0.4)
        

        col *=  amb + 0.25*dif;
    }

    return col*intensity;
}



void main() {
    uint gid = (gl_WorkGroupID.z*gl_NumWorkGroups.y*gl_NumWorkGroups.x
              + gl_WorkGroupID.y*gl_NumWorkGroups.x
              + gl_WorkGroupID.x);
    gid *= gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;
    gid += gl_LocalInvocationIndex; // I think order matters to avoid cache misses

    vec2 frag_coords = vec2(gid/int(imsize.y), gid%int(imsize.y));
    // add check for end of array
    
    vec2 p = (-imsize.xy + 2.0*frag_coords)/imsize.y;
    
    const float fl = -1.5;
    // create view ray
    // vec3 rd = normalize( p.x*uu + p.y*vv + 1.5*ww );
    vec3 ro = view[3].xyz;
    vec3 rd = mat3(view) * normalize(vec3(p, fl)); // mat3() will take the top lefthand corner

    // we will have to move the world, and keep the camera still
    // Work out how to calculate the correction new direction
    vec4 kprime = B*vec4(rd, 1.0); // kprime^2 = 0
    
    // shift the world to make it look like the ray orgin is moving
    rd = normalize(kprime.xyz + B[3].xyz);
    // rd = (kprime.xyz + B[3].xyz)*inversesqrt(1.0+B[3].w*B[3].w*c*c);
    

    // we want the rays for the pixels one above and one two the left to a noise free grid
    vec2 px = (2.0*(frag_coords+vec2(1.0,0.0))-imsize.xy)/imsize.y;
    vec2 py = (2.0*(frag_coords+vec2(0.0,1.0))-imsize.xy)/imsize.y;
    vec3 rdx = mat3(view) * normalize(vec3(px,fl));
    vec3 rdy = mat3(view) * normalize(vec3(py,fl));
    rdx =  normalize((B*vec4(rdx, 1.0)).xyz + B[3].xyz);
    rdy =  normalize((B*vec4(rdy, 1.0)).xyz + B[3].xyz);
     
    vec3 col  = render(ro, rd, rdx, rdy, kprime.w);
   
    
    imageStore(img, ivec2(frag_coords), vec4(sqrt(col), 1.0));
}

