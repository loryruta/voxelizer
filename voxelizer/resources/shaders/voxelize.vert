#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 color;

uniform mat4 u_mesh_transform;
uniform mat4 u_transform;

out vec3 v_position;
out vec3 v_normal;
out vec2 v_uv;
out vec4 v_color;

void main()
{
    gl_Position = u_transform * u_mesh_transform * vec4(position, 1);

	v_position = gl_Position.xyz;
	v_normal = normal;
	v_uv = uv;
	v_color = color;
}
