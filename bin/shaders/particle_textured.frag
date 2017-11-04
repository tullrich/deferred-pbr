#version 130

in vec2 Texcoord;
in vec4 Color;

uniform sampler2D Texture;
#ifdef SOFT_PARTICLES
uniform sampler2D GBuffer_Depth;
#endif

const float ZNear = 0.1;
const float ZFar = 100.0;

out vec4 outColor;

float saturate(float x)
{
	return max(0, min(1, x));
}

float linearizeDepth(float depth)
{
	return (2.0 * ZFar * ZNear) / (ZFar + ZNear - (ZFar - ZNear) * (2.0 * depth - 1.0 ) );
}

void main()
{
	vec4 final = texture(Texture, Texcoord) * Color;
	#ifdef SOFT_PARTICLES
	vec2 screenUV = vec2(gl_FragCoord.x / 1024.0, gl_FragCoord.y / 640.0);
	float particleDepth = linearizeDepth(gl_FragCoord.z);
	float sceneDepth = linearizeDepth(texture(GBuffer_Depth, screenUV).x);
	float softScale = saturate(sceneDepth - particleDepth);
	final.a *= softScale;
	#endif
	outColor = final;
}
