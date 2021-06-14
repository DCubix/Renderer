R""(

uniform vec2 uDirection;
uniform vec2 uResolution;
uniform sampler2D rtRendered;

vec4 filterMain() {
	vec4 color = vec4(0.0);
	vec2 off1 = vec2(1.3333333333333333) * uDirection;
	color += texture2D(rtRendered, uTexCoord, 1) * 0.29411764705882354;
	color += texture2D(rtRendered, uTexCoord + (off1 / uResolution), 1) * 0.35294117647058826;
	color += texture2D(rtRendered, uTexCoord - (off1 / uResolution), 1) * 0.35294117647058826;
	return color; 
}

)""