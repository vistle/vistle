// CubemapReprojectionGridEdge_ps.glsl

in vec3 Direction;

out vec4 ResultColor;

uniform samplerCube ColorTexture;

void main()
{
	ResultColor = vec4(texture(ColorTexture, Direction).xyz, 1.0f);
}