// ShowDepthBuffer_ps.glsl

#extension GL_EXT_gpu_shader4 : enable

in vec2 TexC;

varying out vec4 ResultColor;

uniform sampler2D DepthTextures[6];

float GetDepth(vec2 texCoord, uint regionIndex)
{
	return texture2D(DepthTextures[regionIndex], texCoord).x;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const float c_OneOverThree = 0.333333f;
const float c_TwoOverThree = 0.666667f;

float Gauss(float middle, float dev, float x)
{
	float dist = middle - x;
	float d = dist * dist;
	float divisor = 2.0f * dev * dev;
	return exp(-d / divisor);
}

vec3 VisualizeDepth(float depth)
{
	float value = pow(1.0f - depth, 0.1f);

	// Color coding value.
	float w = Gauss(0.2f, 0.1f, value);
	float b = Gauss(0.4f, 0.1f, value);
	float g = Gauss(0.6f, 0.1f, value);
	float r = Gauss(0.8f, 0.1f, value);

	return vec3(r, g, b) + w;
}

vec4 GetColor(vec2 texCoord, uint regionIndex)
{
	return vec4(VisualizeDepth(GetDepth(texCoord, regionIndex)), 1.0f);
}

void AddLine(float texCoord, float lineValue, float lineWidth, vec3 lineColor, inout vec4 color)
{
	color += smoothstep(0.0f, lineWidth, lineWidth - abs(texCoord - lineValue)) * vec4(lineColor, 0.0f);
}

void GetRegionIndexAndTextureCoord(vec2 texCoord, out uint regionIndex, out vec2 imTexCoord)
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