// CubemapReprojectionGrid_vs.glsl

// Per vertex.
in vec2 TextureCoordinate;

// Per instance.
in vec2 TexCoordOffset;
in uint RegionIndexIn;

out vec2 TexC;
flat out uint RegionIndex;

uniform mat4 Transformations[6];
uniform sampler2DArray DepthTexture;

const float LastDepth = 0.999999f;

void main()
{
	vec2 texCoord = TextureCoordinate + TexCoordOffset;
	uint regionIndex = RegionIndexIn;

	float depth = textureLod(DepthTexture, vec3(texCoord, regionIndex), 0.0f).x;
	gl_Position = Transformations[regionIndex] * vec4(texCoord * 2.0f - 1.0f, depth, 1.0f);
	gl_Position.z = min(gl_Position.z, gl_Position.w * LastDepth);
	TexC = texCoord;
	RegionIndex = regionIndex;
}