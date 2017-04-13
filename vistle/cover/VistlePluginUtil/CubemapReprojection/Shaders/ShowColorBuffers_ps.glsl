// ShowColorBuffer_ps.glsl

#extension GL_EXT_gpu_shader4 : enable

varying vec2 TexC;

uniform samplerCube ColorTexture;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const float c_OneOverThree = 0.333333f;
const float c_TwoOverThree = 0.666667f;

vec4 GetColor(vec2 texCoord, unsigned int regionIndex)
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

void AddLine(float texCoord, float lineValue, float lineWidth, vec3 lineColor, inout vec4 color)
{
	color += smoothstep(0.0f, lineWidth, lineWidth - abs(texCoord - lineValue)) * vec4(lineColor, 0.0f);
}

void GetRegionIndexAndTextureCoord(vec2 texCoord, out unsigned int regionIndex, out vec2 imTexCoord)
{
	if     (texCoord.x <= c_OneOverThree) { regionIndex = 0U;  imTexCoord.x = texCoord.x;                   }
	else if(texCoord.x <= c_TwoOverThree) { regionIndex = 1U;  imTexCoord.x = texCoord.x - c_OneOverThree;  }
	else                                  { regionIndex = 2U;  imTexCoord.x = texCoord.x - c_TwoOverThree;  }
	if     (texCoord.y <= 0.5f)           { regionIndex += 3U; imTexCoord.y = texCoord.y;                   }
	else                                  {                    imTexCoord.y = texCoord.y - 0.5f;            }
	imTexCoord *= vec2(3.0f, 2.0f);
}

void main()
{
	unsigned int regionIndex;
	vec2 imTexCoord;
	GetRegionIndexAndTextureCoord(TexC, regionIndex, imTexCoord);

	vec4 color = GetColor(imTexCoord, regionIndex);
	
	float lineWidth = 0.002f;
	vec3 lineColor = vec3(0.0f, 0.0f, 1.0f);
	AddLine(TexC.x, c_OneOverThree, lineWidth, lineColor, color);
	AddLine(TexC.x, c_TwoOverThree, lineWidth, lineColor, color);
	AddLine(TexC.y, 0.5f,           lineWidth, lineColor, color);
	
	gl_FragColor = color;
}