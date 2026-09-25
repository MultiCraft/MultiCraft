uniform sampler2D baseTexture;

uniform vec4 skyBgColor;
// Colour the node is drawn with, white unless a mod picked one
uniform vec3 nodeColor;
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
varying vec3 eyeVec;
#if POINT_LIGHTS
// Light sources, one per column: xyz in view space, w = level / 15
uniform mat4 pointLights;
#ifdef SETTINGS_UNIFORMS
// Lighting curve as the header spells it, LIGHT_SCALE in the w of the boost
uniform vec4 lightCurve;
uniform vec4 lightBoost;
// Copies main fills, since a Metal function cannot take a constant buffer by reference
vec4 curve;
vec4 boost;
#define LIGHT_CURVE curve
#define LIGHT_BOOST boost.xyz
#define LIGHT_SCALE boost.w
#endif
// Maximum with a rounded corner, a plain max() shows the rim as a crease
vec3 softMax(vec3 a, vec3 b)
{
	const float k = 0.08;
	vec3 h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
	return mix(a, b, h) + k * h * (1.0 - h);
}

float pointLight(vec4 light, vec3 pos)
{
	// One level per node, like spreading node light, but round
	float level = light.w - length(light.xyz - pos) / 150.0;
	if (level <= 0.0)
		return 0.0;
	// Brightness of that level, mirrors decode_light_f() in light.cpp
	float lit = ((LIGHT_CURVE.x * level + LIGHT_CURVE.y) * level + LIGHT_CURVE.z) * level;
	float spread = level - LIGHT_BOOST.y;
	lit += LIGHT_BOOST.x * exp(LIGHT_BOOST.z * spread * spread);
	lit = clamp(lit, 0.0, 1.0);
	if (LIGHT_CURVE.w != 1.0)
		lit = pow(lit, LIGHT_CURVE.w);
	return lit * varColor.a * LIGHT_SCALE;
}
#endif

#ifdef SETTINGS_UNIFORMS
uniform float fogShadingParameter;
#else
const float fogStart = FOG_START;
const float fogShadingParameter = 1.0 / ( 1.0 - fogStart);
#endif

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

	color = base.rgb * nodeColor;

	vec3 lightColor = varColor.rgb;
#if POINT_LIGHTS
#ifdef SETTINGS_UNIFORMS
	curve = lightCurve;
	boost = lightBoost;
#endif
	// The fragment in view space, as eyeVec carries it for the fog
	vec3 pos = -eyeVec * fogDistance;
	for (int i = 0; i < POINT_LIGHTS; i++) {
		// The sources come brightest first, the absent ones last
		if (pointLights[i].w <= 0.0)
			break;
		float point = pointLight(pointLights[i], pos);
		if (point > 0.0)
			lightColor = softMax(lightColor, vec3(point));
	}
	lightColor = min(lightColor, vec3(1.0));
#endif

	vec4 col = vec4(color.rgb * lightColor, 1.0);

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
	col = vec4(col.rgb, base.a);

	gl_FragColor = col;
}
