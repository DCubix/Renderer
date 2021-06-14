R""(

uniform float uThreshold = 0.9;
uniform sampler2D rtRendered;

vec4 filterMain() {
	vec4 col = texture(rtRendered, uTexCoord);
	float luma = dot(col.rgb, vec3(0.299, 0.587, 0.114));
	return col * mix(0.0, 1.0, step(uThreshold, luma));
}

)""