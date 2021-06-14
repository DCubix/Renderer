R""(

uniform float uThreshold = 0.9;
uniform sampler2D rtRendered;

float brightness(vec3 c) {
	return max(max(c.r, c.g), c.b);
}

float antiFlickerLuma(vec2 uv) {
    ivec2 sz = textureSize(rtRendered, 0);
	vec2 texelSize = 1.0 / vec2(float(sz.x), float(sz.y));

    vec4 d = texelSize.xyxy * vec4(-1.0, -1.0, 1.0, 1.0);
	vec3 s1 = texture(rtRendered, uv + d.xy).rgb;
    vec3 s2 = texture(rtRendered, uv + d.zy).rgb;
    vec3 s3 = texture(rtRendered, uv + d.xw).rgb;
    vec3 s4 = texture(rtRendered, uv + d.zw).rgb;

    // Karis's luma weighted average (using brightness instead of luma)
    float s1w = 1.0 / (brightness(s1) + 1.0);
    float s2w = 1.0 / (brightness(s2) + 1.0);
    float s3w = 1.0 / (brightness(s3) + 1.0);
    float s4w = 1.0 / (brightness(s4) + 1.0);
    float one_div_wsum = 1.0 / (s1w + s2w + s3w + s4w);

    return one_div_wsum;
}

vec4 filterMain() {
    vec4 col = texture(rtRendered, uTexCoord);
	float luma = antiFlickerLuma(uTexCoord);

    luma = smoothstep(clamp(uThreshold - 0.5, 0.0, 1.0), uThreshold, luma);

	return col * luma;
}

)""