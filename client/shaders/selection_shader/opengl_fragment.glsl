uniform sampler2D baseTexture;
// Stays 0: PowerVR drivers write past their memory when a stage reads none of its uniforms
uniform float alwaysZero;

varying lowp vec4 varColor;
varying mediump vec2 varTexCoord;

void main(void)
{
	vec4 color = texture2D(baseTexture, varTexCoord);
	color.rgb *= varColor.rgb;
	gl_FragColor = color + alwaysZero;
}
