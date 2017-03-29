// ShowColorBuffer_ps.glsl

in vec2 TexC;

out vec4 ResultColor;

uniform samplerCube ColorTexture;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const float c_OneOverThree = 0.333333f;
const float c_TwoOverThree = 0.666667f;

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

void AddLine(float texCoord, float lineValue, float lineWidth, vec3 lineColor, inout vec4 color)
{
	color += smoothstep(0.0f, lineWidth, lineWidth - abs(texCoord - lineValue)) * vec4(lineColor, 0.0f);
}

void GetRegionIndexAndTextureCoord(vec2 texCoord, out uint regionIndex, out vec2 imTexCoord)
{
	if     (texCoord.x <= c_OneOverThree) { regionIndex = 0;  imTexCoord.x = texCoord.x;                   }
	else if(texCoord.x <= c_TwoOverThree) { regionIndex = 1;  imTexCoord.x = texCoord.x - c_OneOverThree;  }
	else                                  { regionIndex = 2;  imTexCoord.x = texCoord.x - c_TwoOverThree;  }
	if     (texCoord.y <= 0.5f)           { regionIndex += 3; imTexCoord.y = texCoord.y;                   }
	else                                  {                   imTexCoord.y = texCoord.y - 0.5f;            }
	imTexCoord *= vec2(3.0f, 2.0f);
}

void main()
{
	uint regionIndex;
	vec2 imTexCoord;
	GetRegionIndexAndTextureCoord(TexC, regionIndex, imTexCoord);

	vec4 color = GetColor(imTexCoord, regionIndex);
	
	float lineWidth = 0.002f;
	vec3 lineColor = vec3(0.0f, 0.0f, 1.0f);
	AddLine(TexC.x, c_OneOverThree, lineWidth, lineColor, color);
	AddLine(TexC.x, c_TwoOverThree, lineWidth, lineColor, color);
	AddLine(TexC.y, 0.5f,           lineWidth, lineColor, color);
	
	ResultColor = color;
}