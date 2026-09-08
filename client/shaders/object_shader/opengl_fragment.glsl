uniform sampler2D baseTexture;

uniform vec4 emissiveColor;
uniform vec4 skyBgColor;
uniform mediump float fogDistance;
#ifdef USE_ALPHA_REF_DISCARD
uniform float alphaRef;
#elif defined(USE_ALPHA_TEST_DISCARD)
#define alphaRef 0.5
#endif

varying lowp vec4 varColor;
#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

varying mediump vec3 eyeVec;
varying float vIDiff;

const float fogStart = FOG_START;
const float fogShadingParameter = 1.0 / (1.0 - fogStart);

#if ENABLE_TONE_MAPPING

/* Hable's UC2 Tone mapping parameters
	A = 0.12;
	B = 0.30;
	C = 0.10;
	D = 0.20;
	E = 0.01;
	F = 0.30;
	W = 11.2;
	equation used:  ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F
*/

// pow(1.25 * curve(5.5 * pow(c, 2.2)), 1 / 1.6) as 1 polynomial over [0, 1], off by 2 of 255 at most in mediump
vec4 applyToneMapping(vec4 color)
{
	vec3 c = clamp(color.rgb, 0.0, 1.0);
	c = ((((-1.65916 * c + 5.95602) * c - 8.01241) * c + 4.24287) * c + 0.374687) * c - 0.000941302;
	return vec4(max(c, 0.0), color.a);
}
#endif

void main(void)
{
	vec3 color;

	vec4 base = texture2D(baseTexture, varTexCoord).rgba;

#if defined(USE_ALPHA_REF_DISCARD) || defined(USE_ALPHA_TEST_DISCARD)
	if (!(base.a > alphaRef)) {
		discard;
	}
#endif

	color = base.rgb;

	vec4 col = vec4(color.rgb, base.a);

	col.rgb *= varColor.rgb;

	col.rgb *= emissiveColor.rgb * vIDiff;

#if ENABLE_TONE_MAPPING
	col = applyToneMapping(col);
#endif

	// Due to a bug in some (older ?) graphics stacks (possibly in the glsl compiler ?),
	// the fog will only be rendered correctly if the last operation before the
	// clamp() is an addition. Else, the clamp() seems to be ignored.
	// E.g. the following won't work:
	//      float clarity = clamp(fogShadingParameter
	//		* (fogDistance - length(eyeVec)) / fogDistance), 0.0, 1.0);
	// As additions usually come for free following a multiplication, the new formula
	// should be more efficient as well.
	// Note: clarity = (1 - fogginess)
	if (fogDistance > 0.0) { // -1.0 means disabled
		float clarity = clamp(fogShadingParameter
			- fogShadingParameter * length(eyeVec), 0.0, 1.0);
		col = mix(skyBgColor, col, clarity);
	}

	gl_FragColor = vec4(col.rgb, base.a);
}
