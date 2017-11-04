#version 130

in vec3 position;
in vec2 texcoord;

out vec2 Texcoord;

void main()
{
	Texcoord = texcoord;
	gl_Position = vec4( position.xy, 0.5, 1.0 );
}
