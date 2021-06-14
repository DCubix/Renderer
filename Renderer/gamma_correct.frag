R""(

uniform float uGamma = 2.2;
uniform sampler2D rtRendered;

vec4 filterMain() {
	vec4 col = texture(rtRendered, uTexCoord);
	return vec4(pow(col.rgb, vec3(1.0 / uGamma)), 1.0);
}

)""