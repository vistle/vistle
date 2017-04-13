// CubemapReprojectionGridEdge_ps.glsl

#extension GL_EXT_gpu_shader4 : enable

varying vec3 Direction;

uniform samplerCube ColorTexture;

void main()
{
	gl_FragColor = vec4(textureCube(ColorTexture, Direction).xyz, 1.0f);
}