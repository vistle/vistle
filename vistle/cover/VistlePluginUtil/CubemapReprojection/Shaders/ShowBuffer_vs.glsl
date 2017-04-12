// ShowColorBuffer_vs.glsl

#extension GL_EXT_gpu_shader4 : enable

in vec2 TextureCoordinate;

varying out vec2 TexC;

void main()
{
	gl_Position = vec4(TextureCoordinate * 2.0f - 1.0f, 1.0f, 1.0f);
	TexC = TextureCoordinate;
}