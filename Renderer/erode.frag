R""(

vec2 offsets[9] = vec2[](
	vec2(-1.0,-1.0),
	vec2(0.0,-1.0),
	vec2(1.0,-1.0),
	vec2(-1.0,0.0),
	vec2(0.0,0.0),
	vec2(1.0,0.0),
	vec2(-1.0,1.0),
	vec2(0.0,1.0),
	vec2(1.0,1.0)
);

uniform sampler2D rtRendered;
uniform vec2 uResolution;

vec4 filterMain() {
	vec4 col = vec4(1.0);
	vec2 off = 1.0 / uResolution;
    for (int i = 0; i < 9; i++) {
        col = min(texture(rtRendered, uTexCoord + offsets[i] * off), col);
    }
	return col;
}

)""