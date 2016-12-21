#version 130

in vec3 vert;
in vec3 translation;
in vec3 rotation;
in vec3 scale;
in vec2 texcoord;
in vec4 color;

uniform mat4 ModelViewProj;

out vec2 Texcoord;
out vec4 Color;

void main()
{
	vec3 scaled = scale * vert;
	vec3 rotatedX = vec3(scaled.x, scaled.y*cos(rotation.x)+scaled.z*-sin(rotation.x), scaled.y*sin(rotation.x)+scaled.z*cos(rotation.x));
	vec3 rotatedXY = vec3(rotatedX.x*cos(rotation.y)+rotatedX.z*sin(rotation.y), rotatedX.y, rotatedX.x*-sin(rotation.y)+rotatedX.z*cos(rotation.y));
	vec3 rotatedXYZ = vec3(rotatedXY.x*cos(rotation.z)+rotatedXY.y*-sin(rotation.z), rotatedXY.x*sin(rotation.z)+rotatedXY.y*cos(rotation.z), rotatedXY.z);
	gl_Position = ModelViewProj * vec4(translation + rotatedXYZ, 1.0);
	Texcoord = texcoord;
	Color = color;
}
