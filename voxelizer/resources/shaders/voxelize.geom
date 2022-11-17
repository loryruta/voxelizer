#version 460

// Reference:
// https://github.com/otaku690/SparseVoxelOctree/blob/master/WIN/SVO/shader/voxelize.geom.glsl

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 v_position[];
in vec3 v_normal[];
in vec2 v_uv[];
in vec4 v_color[];

out vec3 g_position;
out vec3 g_normal;
out vec2 g_uv;
out vec4 g_color;

flat out int g_axis;

uniform mat4 u_x_ortho_projection;
uniform mat4 u_y_ortho_projection;
uniform mat4 u_z_ortho_projection;

void main()
{
	// Project
	vec3 face_normal = abs(normalize(cross(v_position[1] - v_position[0], v_position[2] - v_position[0])));
	mat4 projection;

	if (face_normal.x > face_normal.y && face_normal.x > face_normal.z)
    {
	    projection = u_x_ortho_projection;
		g_axis = 0;
	}
	else if (face_normal.y > face_normal.x && face_normal.y > face_normal.z)
    {
	    projection = u_y_ortho_projection;
		g_axis = 1;
    }
	else
    {
	    projection = u_z_ortho_projection;
		g_axis = 2;
	}

	vec3 p0 = (projection * vec4(v_position[0], 1)).xyz;
	vec3 p1 = (projection * vec4(v_position[1], 1)).xyz;
	vec3 p2 = (projection * vec4(v_position[2], 1)).xyz;

	// Emit
	gl_Position = vec4(p0, 1);
	g_position = p0;
	g_normal = v_normal[0];
	g_uv = v_uv[0];
	g_color = v_color[0];
	EmitVertex();
	
	gl_Position = vec4(p1, 1);
	g_position = p1;
	g_normal = v_normal[1];
	g_uv = v_uv[1];
	g_color = v_color[1];
	EmitVertex();
	
	gl_Position = vec4(p2, 1);
	g_position = p2;
	g_normal = v_normal[2];
	g_uv = v_uv[2];
	g_color = v_color[2];
	EmitVertex();

	EndPrimitive();
}