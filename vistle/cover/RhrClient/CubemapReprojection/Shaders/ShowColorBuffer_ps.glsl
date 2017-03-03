// ShowColorBuffer_ps.glsl

in vec2 TexC;

out vec4 ResultColor;

uniform uint RegionIndex;
uniform samplerCube ColorTexture;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

vec4 GetColor(vec2 texCoord, uint regionIndex)
{
	vec2 direction2 = texCoord * 2.0f - 1.0f;
	vec3 direction;
	switch (regionIndex)
	{
	case 0: direction = vec3(1.0f, direction2.y, direction2.x); break;
	case 1: direction = vec3(-1.0f, direction2.y, -direction2.x); break;
	case 2: direction = vec3(-direction2.x, 1.0f, -direction2.y); break;
	case 3: direction = vec3(-direction2.x, -1.0f, direction2.y); break;
	case 4: direction = vec3(-direction2.x, direction2.y, 1.0f); break;
	case 5: direction = vec3(direction2, -1.0f); break;
	}

	return vec4(texture(ColorTexture, direction).xyz, 1.0f);
}

void main()
{
	ResultColor = GetColor(TexC, RegionIndex);
}