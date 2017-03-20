// ShowColorBuffer_vs.glsl

in vec2 TextureCoordinate;

out vec2 TexC;

void main()
{
	gl_Position = vec4(TextureCoordinate * 2.0f - 1.0f, 1.0f, 1.0f);
	TexC = TextureCoordinate;
}