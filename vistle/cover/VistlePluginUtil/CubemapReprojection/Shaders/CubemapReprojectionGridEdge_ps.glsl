// CubemapReprojectionGridEdge_ps.glsl

#extension GL_EXT_gpu_shader4 : enable

in vec3 Direction;

varying out vec4 ResultColor;

uniform samplerCube ColorTexture;

void main()
{
	ResultColor = vec4(textureCube(ColorTexture, Direction).xyz, 1.0f);
}