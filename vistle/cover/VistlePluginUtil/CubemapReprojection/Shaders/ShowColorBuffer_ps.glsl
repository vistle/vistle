// DepthComposite_cs.glsl

#extension GL_ARB_gpu_shader4 : enable

in vec2 TexC;

varying out vec4 ResultColor;

uniform sampler2D ColorTexture;

void main()
{
	ResultColor = texture2D(ColorTexture, TexC);
}