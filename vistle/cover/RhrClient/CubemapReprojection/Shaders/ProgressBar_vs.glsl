// ProgressBar_vs.glsl

in vec2 TextureCoordinate;

out vec2 TexC;

uniform uint CountFrames;

const uint FullCountFrames = 120;
const float BottomStartY = -0.8f;

void main()
{
	float ratio = fract(float(CountFrames) / float(FullCountFrames));
	
	gl_Position = vec4(TextureCoordinate.x * 2.0f * ratio - 1.0f, TextureCoordinate.y * (BottomStartY + 1.0f) - 1.0f, 0.0f, 1.0f);
	TexC = TextureCoordinate;
}