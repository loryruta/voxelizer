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

void fix_position(inout uvec3 pos)
{
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
	uint u_block_resolution = 2; // TODO take from extern

	uvec3 pos = uvec3(gl_FragCoord.xy, gl_FragCoord.z * u_viewport);
	fix_position(pos);

	// TODO color could be calculated better
	// TODO uv y is inverted
	vec4 col = g_color * u_color * texture(u_texture2d, vec2(g_uv.x, 1 - g_uv.y));

	vec3 dir = -g_normal;

	vec3 t_max = vec3(0);
	vec3 t_delta = vec3(1) / vec3(dir);
	ivec3 t_step = ivec3(sign(dir));

	for (int i = 0; i < 10; i++)
	{
		if (is_inside_grid(pos))
		{
			push_voxel(pos, col);
		}

		if (t_max.x < t_max.y) {
			if (t_max.x < t_max.z) {
				pos.x += t_step.x;
				t_max.x += t_delta.x;
			} else {
				pos.z += t_step.z;
				t_max.z += t_delta.z;
			}
		} else {
			if (t_max.y < t_max.z) {
				pos.y += t_step.y;
				t_max.y += t_delta.y;
			} else {
				pos.z += t_step.z;
				t_max.z += t_delta.z;
			}
		}
	}
}
