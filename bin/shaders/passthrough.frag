#version 130

in vec2 Texcoord;

uniform sampler2D RenderMap;
uniform sampler2D DepthMap;
#ifdef DEBUG_RENDER_LINEARIZE
uniform float ZNear;
uniform float ZFar;
#endif

out vec4 outColor;

void main()
{
	vec3 color = texture(RenderMap, Texcoord).xyz;
#ifdef DEBUG_RENDER_NORMALIZE
	color = color*0.5f + 0.5f;
#endif
#ifdef DEBUG_RENDER_LINEARIZE
	vec3 ndc = color*2.0f - 1.0f;
	color = 2.0 * ZNear / (ZFar + ZNear - ndc * (ZFar - ZNear));
#endif

	outColor = vec4(color, 1.0f);
	gl_FragDepth = texture(DepthMap, Texcoord).x;
}
