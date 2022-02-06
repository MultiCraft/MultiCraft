uniform sampler2D baseTexture;
uniform sampler2D normalTexture;

#define colorImage baseTexture
#define alphaImage normalTexture

varying mediump vec2 varTexCoord;

void main(void)
{
	vec2 uv = varTexCoord.st;
	vec4 ca = texture2D(colorImage, uv);
	float accum = texture2D(alphaImage, uv).r;
	if (accum == 0.0) {
		discard;
	}
	vec3 color = ca.rgb / accum;
	float alpha = 1.0 - ca.a;
	gl_FragColor = vec4(color, 1.0) * alpha;
}
