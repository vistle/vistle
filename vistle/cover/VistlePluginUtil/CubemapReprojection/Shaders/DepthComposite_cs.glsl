// DepthComposite_cs.glsl

#define LOCAL_SIZE_X 8
#define LOCAL_SIZE_Y 4

layout(local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;

//layout(rgba8)          uniform image2D SourceTargetColor;
//layout(r32f)           uniform image2D SourceTargetDepth;
//layout(rgba8) readonly uniform image2D Source2Color;
//layout(r32f)  readonly uniform image2D Source2Depth;

layout(rgba8) writeonly uniform image2D SourceTargetColor;

uniform int Width;
uniform int Height;

void main()
{
	ivec2 id = ivec2(gl_GlobalInvocationID.xy);
	
	//if (id.x < Width && id.y < Height)
	//{
		/*vec4  color1 = imageLoad(SourceTargetColor, id);
		float depth1 = imageLoad(SourceTargetDepth, id).x;
		vec4  color2 = imageLoad(Source2Color,      id);
		float depth2 = imageLoad(Source2Depth,      id).x;
		
		//vec4  resultColor = mix(color1, color2, float(greaterThan(depth1, depth2)));
		//float resultDepth = min(depth1, depth2);

		vec4  resultColor;
		float resultDepth;
		if(depth1 <= depth2) { resultColor = color1; resultDepth = depth1; }
		else                 { resultColor = color2; resultDepth = depth2; }
		
		resultColor = vec4(0,0,1,1);
		
		imageStore(SourceTargetColor, id, resultColor);
		imageStore(SourceTargetDepth, id, vec4(resultDepth, 0.0f, 0.0f, 0.0f));*/
		
		vec4 resultColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
		imageStore(SourceTargetColor, id, resultColor);
	//}
}