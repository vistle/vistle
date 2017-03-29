// CubemapReprojectionGridEdge_vs.glsl

in vec2 TextureCoordinate;
in uint VertexIndex;

out vec3 Direction;

uniform mat4 Transformations[6];
uniform sampler2DArray DepthTexture;

const float LastDepth = 0.999999f;

vec3 GetDirection(vec2 texCoord, uint regionIndex)
{
	vec2 direction2 = texCoord * 2.0f - 1.0f;
	switch (regionIndex)
	{
	case 0: return vec3(1.0f, direction2.y, direction2.x);
	case 1: return vec3(-1.0f, direction2.y, -direction2.x);
	case 2: return vec3(-direction2.x, 1.0f, -direction2.y);
	case 3: return vec3(-direction2.x, -1.0f, direction2.y);
	case 4: return vec3(-direction2.x, direction2.y, 1.0f);
	case 5: return vec3(direction2, -1.0f);
	}
}

void main()
{
	uint regionIndex = VertexIndex;		
	float depth = textureLod(DepthTexture, vec3(TextureCoordinate, regionIndex), 0.0f).x;
	gl_Position = Transformations[regionIndex] * vec4(TextureCoordinate * 2.0f - 1.0f, depth, 1.0f);
	gl_Position.z = min(gl_Position.z, gl_Position.w * LastDepth);
	Direction = GetDirection(TextureCoordinate, regionIndex);
}