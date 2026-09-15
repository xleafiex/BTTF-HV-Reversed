VSIN(ATTRIB_POS) vec3 in_pos;
VSOUT vec4 v_color;
VSOUT vec2 v_tex0;
VSOUT float v_fog;
VSOUT vec3 v_world;
uniform vec4 leafTime; // seconds, swell amplitude, wind, daylight
// Long-period swell from the user's VCWater shader; world coordinates keep tile seams aligned.
void main() {
    vec4 world=u_world*vec4(in_pos,1.0);
    float swell=sin(dot(world.xy,vec2(.035,.014))-leafTime.x*.9)*.65
               +sin(dot(world.xy,vec2(-.019,.042))-leafTime.x*.71)*.35;
    world.z+=swell*leafTime.y;
    gl_Position=u_proj*u_view*world;
    v_world=world.xyz; v_color=in_color; v_tex0=in_tex0;
    v_fog=DoFog(gl_Position.w);
}
