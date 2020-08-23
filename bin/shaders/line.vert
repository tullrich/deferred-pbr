#version 130

in vec3 position;
in vec3 color;

uniform mat4 ViewProj;

out vec4 Color;

void main()
{
  gl_Position = ViewProj * vec4(position, 1.0);
  Color = vec4(color, 1.0);
}
