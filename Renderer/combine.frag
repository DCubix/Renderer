R""(

uniform sampler2D rtA;
uniform sampler2D rtB;

vec4 filterMain() {
	return texture(rtA, uTexCoord) + texture(rtB, uTexCoord);
}

)""