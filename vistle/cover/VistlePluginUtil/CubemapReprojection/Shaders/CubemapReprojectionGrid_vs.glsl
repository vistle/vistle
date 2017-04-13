// CubemapReprojectionGrid_vs.glsl

#extension GL_EXT_gpu_shader4 : enable

// Per vertex.
attribute vec2 TextureCoordinate;

// Per instance.
attribute vec2 TexCoordOffset;
attribute unsigned int RegionIndex;

varying vec3 Direction;

uniform mat3 TexCoordToDirections[6];
uniform mat4 Transformations[6];
uniform sampler2D DepthTextures[6];

const float LastDepth = 0.999999f;

float GetDepth(vec2 texCoord, unsigned int regionIndex)
{
	return texture2D(DepthTextures[regionIndex], texCoord).x;
}

void main()
{
	vec2 texCoord = TextureCoordinate + TexCoordOffset;

	float depth = GetDepth(texCoord, RegionIndex);
	gl_Position = Transformations[RegionIndex] * vec4(texCoord * 2.0f - 1.0f, depth, 1.0f);
	gl_Position.z = min(gl_Position.z, gl_Position.w * LastDepth);
	Direction = TexCoordToDirections[RegionIndex] * vec3(texCoord, 1.0f);
}