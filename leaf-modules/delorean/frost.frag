uniform sampler2D tex0;
uniform sampler2D tex1;
FSIN float v_snow; FSIN float v_snowEdge;
FSIN vec4 v_color; FSIN vec2 v_tex0; FSIN float v_fog;
FSIN vec3 v_local; FSIN vec3 v_view; FSIN vec3 v_normal; FSIN vec3 v_objectNormal;
// Three planar projections avoid the donor frost UV stretching. The detail
// texture supplies actual fused grains, chipped crust and fine melt channels.
float iceDetail(vec3 p,vec3 weights,float bias){
 return dot(vec3(texture(tex1,p.yz,bias).r,texture(tex1,p.xz,bias).r,texture(tex1,p.xy,bias).r),weights);
}
void main(){
 vec3 weights=pow(abs(normalize(v_objectNormal)),vec3(6.0));
 weights/=max(dot(weights,vec3(1.0)),.0001);
 vec3 p=v_local*.25;
 float detail=iceDetail(p,weights,1.0);
 float fine=iceDetail(v_local*.65+vec3(.31,.63,.17),weights,0.0);
 float mass=iceDetail(p,weights,4.0);
 float sheet=smoothstep(.20,.46,mass);
 float crystals=smoothstep(.31,.67,fine);
 // Sparse dense deposits keep the wet crust visible between snowier patches.
 float deposit=smoothstep(.42,.53,mass)*smoothstep(.24,.42,detail);
 float age=clamp(v_color.a,0.0,1.0);
#ifdef FROST_DOOR_GLASS
 age=1.0;
#endif
 // Thinner areas clear first as the donor timer thaws the crust.
 float thaw=smoothstep((1.0-age)*.52,(1.0-age)*.52+.16,detail);
 float coverage,height;
#ifdef FROST_GLASS
 float drip=texture(tex1,vec2(v_local.x*3.2+v_local.y*2.1,v_local.z*.15)).r;
#ifdef FROST_DOOR_GLASS
 coverage=(.28+.55*sheet+.14*smoothstep(.44,.66,drip))*thaw;
#else
 coverage=(.12+.42*sheet+.10*smoothstep(.44,.66,drip))*thaw*age;
#endif
 height=.00015*detail;
#else
 coverage=mix(mix(.18,.90,sheet),.98,deposit)*thaw*age;
 height=.0015*sheet+.00012*fine+.0045*deposit;
#endif
 vec3 dx=dFdx(v_view),dy=dFdy(v_view);
 vec3 baseNormal=normalize(v_normal),eye=normalize(-v_view);
 if(dot(baseNormal,eye)<0.0)baseNormal=-baseNormal;
 vec3 r1=cross(dy,baseNormal),r2=cross(baseNormal,dx);
 float det=dot(dx,r1);
 vec3 gradient=abs(det)*baseNormal-sign(det)*(dFdx(height)*r1+dFdy(height)*r2);
 vec3 n=dot(gradient,gradient)>1.e-16?normalize(gradient):baseNormal;
 float edge=pow(1.0-max(dot(n,eye),0.0),4.0);
 float microShade=clamp(.92+.22*(n.y-baseNormal.y)+.15*(n.z-baseNormal.z),.65,1.12);
 vec3 illumination=max(v_color.rgb,vec3(.12));
#ifdef FROST_GLASS
 vec3 ice=mix(vec3(.40,.47,.49),vec3(.78,.83,.84),sheet)*illumination;
 ice+=vec3(.12,.15,.16)*edge*illumination;
#else
 // Dense grain scatters light; thin wet regions stay gray and translucent.
 vec3 albedo=vec3(.74,.79,.80)*mix(.70,1.10,detail);
 albedo=mix(albedo,vec3(.94,.96,.96),deposit*.85);
 vec3 ice=albedo*illumination*microShade;
 // Small wet grazing highlights, not an all-over metallic sheen.
 ice+=vec3(.12,.14,.15)*edge*(1.0-sheet*.7)*illumination;
 ice*=mix(.93,1.03,crystals);
#endif
 if(v_snow>.5){
     // Soft thin edges, opaque raised centers; old ice remains underneath.
     float border=smoothstep(.025,.18,v_snowEdge);
     coverage=border*age;
     ice=vec3(.92,.95,.96)*illumination*microShade*mix(.92,1.02,crystals);
 }
 DoAlphaTest(coverage);
 FRAGCOLOR(vec4(mix(u_fogColor.rgb,ice,v_fog),coverage));
}
