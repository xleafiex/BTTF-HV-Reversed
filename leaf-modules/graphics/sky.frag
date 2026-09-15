// GLSL port of the user's VCMiamiSky Atmosphere.hlsl sky and cloud integration.
#define saturate(x) clamp(x,0.0,1.0)
uniform sampler3D Volume;
uniform vec4 Camera, Right, Up, Forward, Projection, Climate, Layer, Solar, Fog, Celestial, AfterRain, RainPatch;
float hash(vec3 p) { return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453); }
vec4 cloudNoise(vec3 p) { return texture(Volume,p); }

float density(vec3 p,bool detail) {
    float h=(p.z-Layer.x)/Layer.y;
    if(h<=0||h>=1)return 0.0;
    vec3 uv=vec3(p.xy/4800+vec2(Climate.x*.0032,Climate.x*.0016),h*.30);
    vec4 n=cloudNoise(uv);
    float weather=cloudNoise(vec3(p.xy/15000+Climate.x*.0008,.73)).r;
    float coverage=saturate(Climate.y+(weather-.5)*.38);
    float body=n.r*.72+n.g*.28;
    // Rounded cumulus tops and broad, flatter bases. Overcast joins the cells.
    float top=mix(.47,.96,saturate(n.r*1.4));
    float profile=smoothstep(0,.13,h)*(1-smoothstep(top*.6,top,h));
    float threshold=mix(.64,.20,coverage);
    float d=saturate((body-threshold-(1-profile)*.25)*6.5);
    if(detail) {
        float erosion=cloudNoise(uv*7+vec3(.1,.8,Climate.x*.0016)).b;
        d=saturate(d-(1-erosion)*.34*(1-d));
    }
    return d*mix(.75,1.4,Climate.z);
}

vec3 atmosphere(vec3 ray) {
    float day=Solar.w, dusk=Celestial.x;
    vec3 zenith=mix(vec3(.006,.012,.033),vec3(.055,.32,.64),day);
    zenith=mix(zenith,vec3(.19,.12,.31),dusk*.6);
    zenith=mix(zenith,vec3(.14,.20,.29)*(.2+.8*day),Climate.z*.7);
    float height=pow(saturate(ray.z),.42);
    vec3 color=mix(Fog.rgb,zenith,height);
    float toward=pow(saturate(dot(ray,Solar.xyz)),8);
    color+=vec3(.33,.16,.055)*toward*day*(1-Climate.z)*(.15+dusk*.7)*smoothstep(0,.12,ray.z);
    // Broad high-altitude wisps: a separate cloudNoise scale, orientation and wind.
    if(ray.z>.015&&Climate.y>0){
        vec2 pos=Camera.xy+ray.xy*((Layer.x+Layer.y+1200-Camera.z)/ray.z);
        vec3 q=vec3(pos.x/22000+Climate.x*.0016,pos.y/6000-Climate.x*.0008,.41);
        float n=cloudNoise(q).a*.7+cloudNoise(q*2+vec3(.37,.19,.61)).b*.3;
        float breakup=cloudNoise(vec3(pos/9000,.21)).r;
        float veil=smoothstep(.49,.72,n)*smoothstep(.3,.6,breakup)*(.03+Climate.y*.18)*(1-Climate.z*.6);
        color=mix(color,mix(vec3(.06,.08,.14),vec3(.80,.85,.9),day),veil);
    }
    if(day<.99&&ray.z>0){
        float angle=Celestial.z*.2617994;
        vec3 r=vec3(ray.x*cos(angle)-ray.y*sin(angle),ray.x*sin(angle)+ray.y*cos(angle),ray.z);
        vec3 cell=floor(r*850), fp=fract(r*850)-.5;
        float star=pow(saturate(1-length(fp)*2),3)*step(.9972,hash(cell));
        color+=star*pow(1-day,3)*Celestial.y*vec3(.7,.8,1)*2;
        float m=dot(ray,-Solar.xyz);
        float moon=smoothstep(.99991,.99996,m);
        float halo=pow(saturate(m),220)*.025;
        // Keep the time-cycle moon opposite the sun so the native moon can be
        // restored without a second, screen-space disc fighting it.
        color+=(moon*.7+halo)*vec3(.70,.79,1)*(1-day)*(1-day);
    }
    // Native lightning is rendered before the replacement sky. Carry its
    // flash through the replacement layer as a front-facing atmospheric hit.
    return mix(color+vec3(.35,.48,.8)*Layer.w*.45,vec3(1), saturate(Layer.w*.72));
}

float SunTransmission(vec3 p){
    float toCloud=max(0,(Layer.x-p.z)/max(Solar.z,.02));
    float shadow=0;
     for(int j=0;j<4;++j)
        shadow+=density(p+Solar.xyz*(toCloud+Layer.y*(.08+j*.20)/max(Solar.z,.02)),true);
    return exp(-shadow*3.2);
}
vec3 SunShafts(vec3 ray,float maximumDistance){
    if(AfterRain.x<=0||Solar.z<.04||Celestial.z<6||Celestial.z>=19||Climate.y<.08||Climate.z>.8)return vec3(0);
    float distance=min(maximumDistance,ray.z>.005?min(6000,max(0,(Layer.x-Camera.z)/ray.z)):6000);
    float illumination=0;
     for(int i=0;i<40;++i){
        float t=distance*(i+.5)/40;
        vec3 p=Camera.xyz+ray*t;
        illumination+=SunTransmission(p)*exp(-t*.00022);
    }
    float forward=.20+.80*pow(saturate(dot(ray,Solar.xyz)),4);
    return vec3(1,.91,.73)*illumination/40*forward*AfterRain.x*.65*(1-exp(-distance*.001))*Solar.w*(1-Climate.z);
}
vec3 Rainbow(vec3 ray){
    if(AfterRain.y<=0||ray.z<=0||Celestial.z<6||Celestial.z>=19||Climate.z>.8)return vec3(0);
    vec3 sun=Solar.xyz;
    if(sun.z<=0||sun.z>=.669131)return vec3(0);
    float angle=acos(clamp(dot(ray,-sun),-1,1));
    if(angle<.68||angle>.75)return vec3(0);
    const float radius[7]=float[7](.7330,.7272,.7214,.7156,.7098,.7040,.6982);
    const vec3 colors[7]=vec3[7](vec3(1,.08,.02),vec3(1,.38,.02),vec3(1,.85,.04),vec3(.12,.7,.08),vec3(.05,.46,1),vec3(.22,.14,.8),vec3(.53,.12,.72));
    vec3 bow=vec3(0);
     for(int i=0;i<7;++i)bow+=colors[i]*exp(-pow((angle-radius[i])/.0045,2));
    // A rain curtain is world-anchored for this event, independent of cloud wind.
    float t=max(200,dot(RainPatch.xy-Camera.xy,ray.xy)/max(dot(ray.xy,ray.xy),.01));
    vec3 rainPoint=Camera.xyz+ray*t;
    float curtain=1-smoothstep(RainPatch.z*.55,RainPatch.z,length(rainPoint.xy-RainPatch.xy));
    float shower=cloudNoise(vec3(rainPoint.xy/1700,RainPatch.w)).r;
    float droplets=curtain*smoothstep(.12,.42,shower)*(1-smoothstep(Layer.x,Layer.x+Layer.y*.5,rainPoint.z));
    // Cloud shadows cut illuminated rain into fragments and radial light/dark spokes.
    float direct=SunTransmission(rainPoint);
    return bow*AfterRain.y*.35*droplets*direct*smoothstep(0,.018,ray.z)*Solar.w;
}
vec4 FoggedSky(vec3 color,vec3 ray){
    // The skyline must dissolve into the same atmosphere as distant buildings.
    // Use the same low horizon ramp for sky, city fog and water so rain/cloud
    // transitions do not leave a bright one-pixel skyline gap.
    float horizon=1-smoothstep(.025,.34,saturate(ray.z));
    float mist=pow(Climate.w,3)*(.88+.12*horizon);
    vec3 blended=saturate(mix(color,Fog.rgb,max(horizon,mist))+SunShafts(ray,6000)*smoothstep(0,.055,ray.z)*(1-mist*.6)+Rainbow(ray)*(1-mist));
    return vec4(mix(blended,vec3(1),saturate(Layer.w*.82)),1);
}
vec4 SkyPS(vec2 uv) {
    vec2 ndc=vec2(uv.x*2-1,1-uv.y*2);
    vec3 ray=normalize(Forward.xyz+Right.xyz*ndc.x*Projection.x+Up.xyz*ndc.y*Projection.y);
    vec3 background=atmosphere(ray);
    if(Climate.y<.01||abs(ray.z)<.004)return FoggedSky(background,ray);
    float a=(Layer.x-Camera.z)/ray.z,b=(Layer.x+Layer.y-Camera.z)/ray.z;
    float entry=max(0,min(a,b)),leave=min(26000,max(a,b));
    if(leave<=entry)return FoggedSky(background,ray);
    float stepSize=(leave-entry)/Layer.z;
    // Fixed subpixel dither avoids animated cloudNoise and hides low-resolution bands.
    float jitter=fract(dot(floor(uv*Projection.zw),vec2(.75487766,.56984029)));
    float t=entry+(jitter*.7+.15)*stepSize;
    float trans=1;
    vec3 light=vec3(0);
    float phase=.65+1.5*pow(saturate(dot(ray,Solar.xyz)),6);
    vec3 sunColor=mix(vec3(1,.49,.27),vec3(1,.96,.87),saturate(Solar.z*3));
    vec3 ambient=mix(vec3(.032,.047,.082),vec3(.49,.57,.69),Solar.w);
     for(int i=0;i<80;++i){
        if(i>=Layer.z||trans<.012)break;
        vec3 p=Camera.xyz+ray*t;
        float d=density(p,true);
        if(d>.004){
            float shadow=0;
             for(int j=1;j<=3;++j){
                shadow+=density(p+Solar.xyz*(j*Layer.y*.16),false);
            }
            float lighting=exp(-shadow*2.7);
            float h=saturate((p.z-Layer.x)/Layer.y);
            float powder=1-exp(-d*3);
            float cloudDay=1-saturate(max(0,Climate.y-.18)*.58);
            vec3 illumination=ambient*mix(.68,1.0,h)*(1-Climate.z*.4)+sunColor*Solar.w*lighting*(.9+powder*.45)*cloudDay;
            illumination+=vec3(.48,.62,1)*Layer.w*(.45+lighting);
            float extinction=d*stepSize*.008;
            float alpha=1-exp(-extinction);
            // Distant cloud bases merge into exactly the same horizon as city/water fog.
            illumination=illumination/(vec3(1)+illumination*.35)*1.08;
            float haze=max(1-exp(-t*(.000045+Climate.w*.000055)),smoothstep(11000,24000,t));
            illumination=mix(illumination,Fog.rgb,haze);
            light+=trans*alpha*illumination;
            trans*=1-alpha;
        }
        t+=stepSize;
    }
    return FoggedSky(saturate(light+background*trans),ray);
}

void main(){vec2 uv=gl_FragCoord.xy/Projection.zw;uv.y=1.0-uv.y;FRAGCOLOR(SkyPS(uv));}
