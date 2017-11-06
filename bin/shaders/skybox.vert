#version 130

in vec3 position;

uniform vec3 CameraPos;
uniform mat4 InvViewProj;

out vec3 TexDir;

void main()
{
	vec4 nearPoint = InvViewProj*vec4(position.xy, 1, 1);
	nearPoint /= nearPoint.w;
	TexDir = position;//nearPoint.xyz-CameraPos;
	gl_Position = InvViewProj * vec4(position, 1);
}
