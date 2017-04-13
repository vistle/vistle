// CubemapReprojectionGridEdge_vs.glsl

#extension GL_EXT_gpu_shader4 : enable

attribute vec2 TextureCoordinate;
attribute unsigned int VertexIndex;

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
	unsigned int regionIndex = VertexIndex;
	float depth = GetDepth(TextureCoordinate, regionIndex);
	gl_Position = Transformations[regionIndex] * vec4(TextureCoordinate * 2.0f - 1.0f, depth, 1.0f);
	gl_Position.z = min(gl_Position.z, gl_Position.w * LastDepth);
	Direction = TexCoordToDirections[regionIndex] * vec3(TextureCoordinate, 1.0f);
}