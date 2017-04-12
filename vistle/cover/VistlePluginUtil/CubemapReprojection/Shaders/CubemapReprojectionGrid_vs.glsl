// CubemapReprojectionGrid_vs.glsl

#extension GL_EXT_gpu_shader4 : enable

// Per vertex.
in vec2 TextureCoordinate;

// Per instance.
in vec2 TexCoordOffset;
in uint RegionIndexIn;

varying out vec2 TexC;
flat varying out uint RegionIndex;

uniform mat4 Transformations[6];
uniform sampler2D DepthTextures[6];

const float LastDepth = 0.999999f;

float GetDepth(vec2 texCoord, uint regionIndex)
{
	return texture2D(DepthTextures[regionIndex], texCoord).x;
}

void main()
{
	vec2 texCoord = TextureCoordinate + TexCoordOffset;
	uint regionIndex = RegionIndexIn;

	float depth = GetDepth(texCoord, regionIndex);
	gl_Position = Transformations[regionIndex] * vec4(texCoord * 2.0f - 1.0f, depth, 1.0f);
	gl_Position.z = min(gl_Position.z, gl_Position.w * LastDepth);
	TexC = texCoord;
	RegionIndex = regionIndex;
}