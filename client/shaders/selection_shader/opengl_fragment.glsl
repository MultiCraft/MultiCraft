uniform sampler2D baseTexture;

varying lowp vec4 varColor;
varying mediump vec2 varTexCoord;

void main(void)
{
	vec4 color = texture2D(baseTexture, varTexCoord);
	color.rgb *= varColor.rgb;
	gl_FragColor = color;
}
