// ShowColorBuffer_vs.glsl

in vec2 TextureCoordinate;

out vec2 TexC;

const float c_Depth = 0.5f;

void main()
{
	gl_Position = vec4(TextureCoordinate * 2.0f - 1.0f, c_Depth, 1.0f);
	TexC = TextureCoordinate;
}