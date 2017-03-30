// DepthComposite_cs.glsl

#define LOCAL_SIZE_X 8
#define LOCAL_SIZE_Y 4

layout(local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;

layout(rgba8)          uniform image2D   SourceTargetColor;
                       uniform sampler2D SourceTargetDepth;
layout(rgba8) readonly uniform image2D   Source2Color;

uniform int Width;
uniform int Height;
uniform vec2 TCMult;

void main()
{
	ivec2 id = ivec2(gl_GlobalInvocationID.xy);
	
	if (id.x < Width && id.y < Height)
	{
		vec2 texCoord = (vec2(id) + 0.5f) * TCMult;

		vec4  color1 = imageLoad(SourceTargetColor, id);
		float depth  = texture  (SourceTargetDepth, texCoord).x;
		vec4  color2 = imageLoad(Source2Color,      id);
		
		vec4  resultColor = mix(color1, color2, float(depth >= 1.0f));
		
		imageStore(SourceTargetColor, id, resultColor);
	}
}