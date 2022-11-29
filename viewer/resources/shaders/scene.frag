#version 430

in vec2 v_tex_coord;
in vec4 v_color;

uniform vec4 u_color;
uniform sampler2D u_texture2d;

out vec4 f_color;

void main()
{
	f_color = u_color * v_color * texture(u_texture2d, vec2(v_tex_coord.x, 1 - v_tex_coord.y));
}
