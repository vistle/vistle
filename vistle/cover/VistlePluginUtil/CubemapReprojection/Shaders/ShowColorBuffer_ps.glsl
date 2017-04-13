// DepthComposite_cs.glsl

#extension GL_ARB_gpu_shader4 : enable

varying vec2 TexC;

uniform sampler2D ColorTexture;

void main()
{
	gl_FragColor = texture2D(ColorTexture, TexC);
}