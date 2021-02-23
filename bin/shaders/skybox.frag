#version 130

#define GAMMA 2.2

in vec3 TexDir;

uniform samplerCube SkyboxCube;
uniform float Lod;

out vec4 outColor;

vec3 Uncharted2ToneMapping(vec3 color)
{
  float A = 0.22;//0.15;
  float B = 0.30;//0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.01;//0.02;
  float F = 0.30;//0.30;
  float W = 11.2;
  float exposure = 2.;
  color *= exposure;
  color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
  float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
  color /= white;
  color = pow(color, vec3(1. / GAMMA));
  return color;
}

void main()
{
  vec3 color = pow(textureLod(SkyboxCube, TexDir, Lod).xyz, vec3(GAMMA));
  outColor = vec4(Uncharted2ToneMapping(color), 1);
}
