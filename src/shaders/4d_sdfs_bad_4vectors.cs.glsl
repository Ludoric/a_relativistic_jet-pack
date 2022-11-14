#version 430
layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) restrict writeonly uniform image2D img;

uniform sampler2D lambda2RGB;

uniform mat4 view;
uniform mat4 B;  // Lorentz boost matrix from the camera's frame
uniform vec2 imsize;
// uniform float tau; // camera time 
uniform float time; // world time

const float C = 32.0;


vec2 union_ab( vec2 a, vec2 b ){
	return (a.x<b.x) ? a : b;
}

vec2 map4(in vec4 pin){
    // vec3 p = mod(pin+vec3(1.0), vec3(2.0))-vec3(1.0);
    const vec2 c = vec2(10.0, 2.0);
    vec3 p = vec3(mod(pin.x+0.5*c.x - 1.0, c.x)-0.5*c.x, pin.y-0.5, mod(pin.z+0.5*c.y,c.y)-0.5*c.y);
    
    const vec2 torus_dim = vec2(0.5, 0.25);
    vec2 q = vec2(length(p.xz)-torus_dim.x,p.y);
    vec2 torus = vec2(length(q)-torus_dim.y, 1.0);
    
    // 0.001*C ? 
    p.xz = vec2(mod(pin.x+0.5*c.x - 5.0, c.x)-0.5*c.x, mod(pin.z+0.5*c.y - 0.001*pin.w ,c.y)-0.5*c.y);
    vec3 qb = abs(p) - vec3(0.5);
    // first three is velocity
    vec2 boxes =  vec2(length(max(qb,0.0)) + min(max(qb.x,max(qb.y,qb.z)),0.0), 2.0);

    return union_ab(torus, boxes);
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
vec2 raycast(in vec4 ro, in vec4 rd){
    vec2 res = vec2(-2.0, -1.0);// raymarch
    float tmin = 0.2;
    float tmax = 80.0;

    // raytrace floor plane
    float tp1 = (0.0-ro.y)/rd.y;
    if (tp1<0.0){
        tmax = min(tmax, tp1);
        res = vec2(tp1, 0.0);
    }

    if ((abs(ro.y) < 2.0) || (rd.y*ro.y>0)){
        float t = tmin;
        for (int i=0; i<192 && t<tmax; i++){
            vec2 h = map4(ro + t*rd);
            if (abs(h.x) < (0.0001*t)){
                res = vec2(t, h.y);
                break;
            }
            t += h.x;
        }
    }
    return res;
}


// // https://iquilezles.org/articles/normalsSDF
// // It's nothing very exciting; some well tuned values and tetrahedral sampling
// vec3 calcNormal(in vec3 pos ){
//     vec2 e = vec2(1.0,-1.0)*0.5773;
//     const float eps = 0.0005;
//     return normalize(e.xyy*map(pos + e.xyy*eps).x + 
// 					 e.yyx*map(pos + e.yyx*eps).x + 
// 					 e.yxy*map(pos + e.yxy*eps).x + 
// 					 e.xxx*map(pos + e.xxx*eps).x);
// }

vec4 calcNormal4(in vec4 p ){
    // // vec3 e = vec3((3.0*sqrt(5.0)-1.0)/8.0, (-sqrt(5.0)-1.0)/8.0, 1.0/2.0);
    // vec3 e = vec3(0.713525491562421, -0.404508497187474, 0.5);
    // const float eps = 0.0005;
    // vec4 v = (e.xyyy*map4(p + e.xyyy*eps).x +
    //           e.yxyy*map4(p + e.yxyy*eps).x +
    //           e.yyxy*map4(p + e.yyxy*eps).x +
    //           e.yyyx*map4(p + e.yyyx*eps).x +
    //           e.zzzz*map4(p + e.zzzz*eps).x);
    // const vec3 e = vec3(sqrt(5.0)/4.0, -sqrt(5.0)/4.0 , -1/4.0);
    const vec3 e = vec3(0.559016994374947, -0.559016994374947, -0.25);
    const vec4 f = vec4(0.0, 0.0, 0.0, 1.0);
    const float eps = 0.0005;
    vec4 v = (e.xxxz*map4(p + e.xxxz*eps).x +
              e.yyyz*map4(p + e.yyyz*eps).x +
              e.yxyz*map4(p + e.yxyz*eps).x +
              e.yyxz*map4(p + e.yyxz*eps).x +
              f*map4(p + f*eps).x);
    return normalize(v);  // v*inversesqrt(v.xyz);
}


mat4 boost(in vec3 v){
    const float oc = 1.0/C;
    const float v2 = dot(v,v);
    const float g = inversesqrt(1.0 - v2*oc*oc);
    const float gm1ov = (g-1.0)/v2;
    return mat4(vec4(1.0+gm1ov*v.x*v.x, gm1ov*v.y*v.x, gm1ov*v.z*v.x, -g*oc*v.x),
                vec4(gm1ov*v.x*v.y, 1.0+gm1ov*v.y*v.y, gm1ov*v.z*v.y, -g*oc*v.y),
                vec4(gm1ov*v.x*v.z, gm1ov*v.y*v.z, 1.0-gm1ov*v.z*v.z, -g*oc*v.z),
                vec4(-g*oc*v.x, -g*oc*v.y, -g*oc*v.z, g));
}


/* 
 * rdx, rdy are solely used by the antialiasing of the checkerboard pattern
 */
vec3 render(in vec4 ro, in vec4 rd, in vec3 rdx, in vec3 rdy, in float wr){
    // background
    vec3 col = vec3(0.7, 0.7, 0.9) - max(rd.y,0.0)*0.3;
    
    // kprime.w will give the change in colour frequency.
    // I/omega^3 = invarient
    float intensity = 0.7/(wr*wr*wr);
    

    vec2 res = raycast(ro, rd);
    float t = res.x;
    float m =  res.y;
    
    if (m > -0.5){
        vec4 pos = ro + t*rd;
        vec4 nor = vec4(0.0, 1.0, 0.0, 0.0); // plane normal

        if (m < 1.0){// it's on the plane
            // more magic from Inigo Quilez
            // project pixel footprint into the plane
            vec3 dpdx = ro.y*(rd.xyz/rd.y-rdx/rdx.y);
            vec3 dpdy = ro.y*(rd.xyz/rd.y-rdy/rdy.y);

            float f = checkersGradBox( 3.0*pos.xz, 3.0*dpdx.xz, 3.0*dpdy.xz);
            col = 0.15 + f*vec3(0.05);
            // ks = 0.4;
        }else{
            nor = calcNormal4(pos);
            // Use a small 3d texture (generated in code?) to do the conversion from velocity shifted color to rgb
            // start with 550nm light (green)
            float c = (m == 1.0) ? 550.0 : 490.0;

            wr = (boost(nor.xyz/nor.w)*rd).w;
            col = texture(lambda2RGB, vec2(((c*wr)-380.0)/(750.0-380.0), 0)).xyz;
            // vec3 c = texture(lambda2RGB, vec2(((550.0*kprime.w)-256.666)/616.666, 0)).xyz;
        }

        // shading/lighting	
        float dif = clamp(dot(nor.xyz, vec3(0.7,0.6,0.4)), 0.0, 1.0);
        float amb = 0.5 + 0.5*dot(nor.xyz, vec3(0.0,0.8,0.6));
        // col = vec3(0.2, 0.3, 0.4)*amb + vec3(0.8,0.7,0.5)*dif;  // vec3(0.2, 0.3, 0.4)
        

        col *=  amb + 0.25*dif;
    }

    return sqrt(col*intensity);
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
    
    const float fl = 1.5;
    // create view ray
    // vec3 rd = normalize( p.x*uu + p.y*vv + 1.5*ww );
    vec4 ro = view[3];
    vec4 rd = vec4(mat3(view) * normalize(vec3(p, fl)), 1.0); // mat3() will take the top lefthand corner
    // the 1.0 should represent C, so we will have to measure time in units of Ct


    // change of coordinates!
    // rd is in the camera frame. We want it in the world frame
    // we lorentz boost each light ray from the camera (kprime)
    vec4 kprime = B*rd; // kprime^2 = 0
    
    // and then have to shift the world to make it look like the ray orgin is moving
    // B[3].xyz = -gamma*v/c, B[3].w = gamma
    rd.xyz = normalize(kprime.xyz/kprime.w + B[3].xyz);
    // rd = normalize(kprime + B[3]);
    // rd = (kprime.xyz + B[3].xyz)*inversesqrt(1.0+B[3].w*B[3].w*c*c);
    // rd should now be in the world frame
    

    // we want the rays for the pixels one above and one two the left to a noise free grid
    vec2 px = (2.0*(frag_coords+vec2(1.0,0.0))-imsize.xy)/imsize.y;
    vec2 py = (2.0*(frag_coords+vec2(0.0,1.0))-imsize.xy)/imsize.y;
    vec3 rdx = mat3(view) * normalize(vec3(px,fl));
    vec3 rdy = mat3(view) * normalize(vec3(py,fl));
    vec4 krdx = B*vec4(rdx, 1.0);
    vec4 krdy = B*vec4(rdy, 1.0);
    rdx =  normalize(krdx.xyz/krdx.w + B[3].xyz);
    rdy =  normalize(krdy.xyz/krdy.w + B[3].xyz);
     
    vec3 col  = render(ro, rd, rdx, rdy, kprime.w);
   
    
    imageStore(img, ivec2(frag_coords), vec4(col, 1.0));
}

