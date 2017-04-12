// CubemapReprojectionGridEdge_vs.glsl

#extension GL_EXT_gpu_shader4 : enable

in vec2 TextureCoordinate;
in uint VertexIndex;

varying out vec3 Direction;

uniform mat4 Transformations[6];
uniform sampler2D DepthTextures[6];

const float LastDepth = 0.999999f;

vec3 GetDirection(vec2 texCoord, uint regionIndex)
{
	vec2 direction2 = texCoord * 2.0f - 1.0f;
	switch (regionIndex)
	{
	case 0U: return vec3(1.0f, direction2.y, direction2.x);
	case 1U: return vec3(-1.0f, direction2.y, -direction2.x);
	case 2U: return vec3(-direction2.x, 1.0f, -direction2.y);
	case 3U: return vec3(-direction2.x, -1.0f, direction2.y);
	case 4U: return vec3(-direction2.x, direction2.y, 1.0f);
	case 5U: return vec3(direction2, -1.0f);
	}
}

float GetDepth(vec2 texCoord, uint regionIndex)
{
	return texture2D(DepthTextures[regionIndex], texCoord).x;
}

void main()
{
	uint regionIndex = VertexIndex;		
	float depth = GetDepth(TextureCoordinate, regionIndex);
	gl_Position = Transformations[regionIndex] * vec4(TextureCoordinate * 2.0f - 1.0f, depth, 1.0f);
	gl_Position.z = min(gl_Position.z, gl_Position.w * LastDepth);
	Direction = GetDirection(TextureCoordinate, regionIndex);
}