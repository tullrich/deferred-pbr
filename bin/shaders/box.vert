#version 130

in vec3 position;
in vec2 texcoord;

out vec2 Texcoord;

uniform mat4 view;

void main()
{
	Texcoord = texcoord;
	gl_Position = view * vec4( position, 1.0 );
}