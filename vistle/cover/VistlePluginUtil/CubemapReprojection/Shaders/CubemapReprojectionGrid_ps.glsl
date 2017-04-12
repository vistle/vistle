// CubemapReprojectionGrid_ps.glsl

#extension GL_EXT_gpu_shader4 : enable

in vec2 TexC;
flat in uint RegionIndex;

varying out vec4 ResultColor;

uniform samplerCube ColorTexture;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

vec4 GetColor(vec2 texCoord, uint regionIndex)
{
	vec2 direction2 = texCoord * 2.0f - 1.0f;
	vec3 direction;
	switch (regionIndex)
	{
	case 0U: direction = vec3(1.0f, direction2.y, direction2.x); break;
	case 1U: direction = vec3(-1.0f, direction2.y, -direction2.x); break;
	case 2U: direction = vec3(-direction2.x, 1.0f, -direction2.y); break;
	case 3U: direction = vec3(-direction2.x, -1.0f, direction2.y); break;
	case 4U: direction = vec3(-direction2.x, direction2.y, 1.0f); break;
	case 5U: direction = vec3(direction2, -1.0f); break;
	}

	return vec4(textureCube(ColorTexture, direction).xyz, 1.0f);
}

void main()
{
	ResultColor = GetColor(TexC, RegionIndex);
}