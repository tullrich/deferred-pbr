#version 130

in vec2 position;

uniform mat4 InvViewProj;

out vec3 TexDir;

void main()
{
	vec4 nearPoint = InvViewProj*vec4(position.xy, 1, 1);
	nearPoint /= nearPoint.w;
	TexDir = nearPoint.xyz;
	gl_Position = vec4(position.xy, 1, 1);
}
