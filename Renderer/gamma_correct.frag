R""(

/*
rtRendered = Last rendered frame
rtNormals = Normals buffer
rtPosition = Position buffer
rtMaterial = Material buffer
rtDiffuse = Diffuse buffer

uTexCoord = Screen-space UVs


*/

vec4 filterMain() {
	vec4 col = texture(rtRendered, uTexCoord);
	return vec4(pow(col.rgb, vec3(1.0 / 2.2)), 1.0);
}

)""