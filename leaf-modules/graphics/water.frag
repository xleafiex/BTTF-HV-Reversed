FSIN vec4 v_color;
FSIN vec2 v_tex0;
FSIN float v_fog;
FSIN vec3 v_world;
uniform sampler2D tex0;
uniform vec4 leafTime;
uniform vec4 leafCamera;
uniform vec4 leafWater; // original configured tint and reflection strength
uniform vec4 leafSun;
void main() {
    float t=leafTime.x;
    vec2 p=v_world.xy;
    // Analytic derivatives of the shared swell plus smaller moving ripples.
    vec2 slope=vec2(.035,.014)*cos(dot(p,vec2(.035,.014))-t*.9)*.65*leafTime.y
              +vec2(-.019,.042)*cos(dot(p,vec2(-.019,.042))-t*.71)*.35*leafTime.y;
    slope+=vec2(.065,.04)*cos(dot(p,vec2(.9,.6))-t*1.3);
    slope+=vec2(-.045,.07)*cos(dot(p,vec2(-.7,1.1))-t*1.1);
    vec3 n=normalize(vec3(-slope,1));
    vec3 view=normalize(leafCamera.xyz-v_world);
    float fresnel=.025+.975*pow(1.0-max(dot(n,view),0.0),5.0);
    vec3 reflected=reflect(-view,n);
    float horizon=pow(clamp(reflected.z,0.0,1.0),.42);
    vec3 sky=mix(u_fogColor.rgb,mix(vec3(.006,.012,.033),vec3(.055,.32,.64),leafTime.w),horizon);
    vec3 base=leafWater.rgb*(.10+.35*leafTime.w)*mix(vec3(.7),v_color.rgb,.25);
    float sparkle=pow(max(dot(reflect(-leafSun.xyz,n),view),0.0),160.0)*leafTime.w;
    vec3 color=mix(base,sky,clamp(fresnel*leafWater.w,0.0,.96))+vec3(1,.91,.73)*sparkle*.65;
    color=mix(u_fogColor.rgb,color,v_fog);
    float alpha=v_color.a*texture(tex0,vec2(v_tex0.x,1.0-v_tex0.y)).a;
    DoAlphaTest(alpha);
    FRAGCOLOR(vec4(color,alpha));
}
