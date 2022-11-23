#version 460

in vec3 g_position;
in vec3 g_normal;
in vec2 g_uv;
in vec4 g_color;

flat in int g_axis;

layout (pixel_center_integer) in vec4 gl_FragCoord;

layout(location = 6) uniform vec4 u_color;
layout(location = 7, binding = 0) uniform sampler2D u_texture2d;

uniform uint u_viewport;
uniform uvec3 u_grid;

uniform uint u_can_store;

layout(binding = 1, rgb10_a2ui) uniform uimageBuffer u_voxel_list_position;
layout(binding = 2, rgba8) uniform imageBuffer u_voxel_list_color;
layout(binding = 3) uniform atomic_uint u_voxels_count;
layout(binding = 4) uniform atomic_uint atomic_errors_counter;

void assert(bool test)
{
	if (!test) {
		atomicCounterIncrement(atomic_errors_counter);
	}
}

bool is_inside_grid(uvec3 pos)
{
	return pos.x < u_grid.x && pos.y < u_grid.y && pos.z < u_grid.z;
}

void push_voxel(uvec3 pos, vec4 col)
{
	uint loc = atomicCounterIncrement(u_voxels_count);
	if (u_can_store == 1)
	{
		imageStore(u_voxel_list_position, int(loc), uvec4(pos, 0));
		imageStore(u_voxel_list_color, int(loc), col);
	}
}

void main()
{
	uvec3 pos = uvec3(gl_FragCoord.xy, gl_FragCoord.z * u_viewport);

	switch (g_axis)
	{
		case 0: // X
			pos.xyz = pos.zyx;
			break;
		case 1: // Y
			pos.xyz = pos.xzy;
			break;
		case 2: // Z
			pos.xyz = uvec3(pos.xy, u_viewport - pos.z);
			break;
		default:
			assert(false);
			break;
	}

	// TODO color could be calculated better
	// TODO uv y is inverted
	vec4 col = g_color * u_color * texture(u_texture2d, vec2(g_uv.x, 1 - g_uv.y));

	if (is_inside_grid(pos))
	{
		push_voxel(pos, col);
	}
}
