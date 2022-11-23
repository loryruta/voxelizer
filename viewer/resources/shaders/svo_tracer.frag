#version 460

#define EPS 3.552713678800501e-15

#define MAX_DEPTH 32

uniform uvec2 u_screen;

uniform vec3 u_position;
uniform mat4 u_projection;
uniform mat4 u_view;

layout(std430, binding = 0) buffer ssbo_octree { uint b_octree[]; };
uniform uint u_start_address; // The starting index within the octree, this is useful whether we need to render a sub-portion of the octree.

uniform vec3 u_octree_from;
uniform vec3 u_octree_size;

out vec4 f_color;

void swap(inout float a, inout float b)
{
	float tmp = a;
	a = b;
	b = tmp;
}

vec4 octree_unpack_color(uint value)
{
	return vec4(value & 0xFFu, (value >> 8u) & 0xFFu, (value >> 16u) & 0xFFu, (value >> 24u) & 0x7Fu);
}

vec4 _3BIT_DEBUG[] = vec4[](
	vec4(0, 0, 0, 1), // 000
	vec4(0, 0, 1, 1), // 001
	vec4(0, 1, 0, 1), // 010
	vec4(0, 1, 1, 1), // 011
	vec4(1, 0, 0, 1), // 100
	vec4(1, 0, 1, 1), // 101
	vec4(1, 1, 0, 1), // 110
	vec4(1, 1, 1, 1)  // 111
);

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool ray_intersect(vec3 o, vec3 d, vec3 _min, vec3 _max, out float t_min, out float t_max)
{
	// x
	t_min = (_min.x - o.x) / d.x;
	t_max = (_max.x - o.x) / d.x;

	if (t_min > t_max)
		swap(t_min, t_max);

	// y
	float ty_min = (_min.y - o.y) / d.y;
	float ty_max = (_max.y - o.y) / d.y;

	if (ty_min > ty_max)
		swap(ty_min, ty_max);

	if (t_min > ty_max || ty_min > t_max)
		return false;

    if (ty_min > t_min) t_min = ty_min; 
    if (ty_max < t_max) t_max = ty_max; 

	// z
	float tz_min = (_min.z - o.z) / d.z;
	float tz_max = (_max.z - o.z) / d.z;

	if (tz_min > tz_max)
		swap(tz_min, tz_max);

	if (t_min > tz_max || tz_min > t_max)
		return false;

    if (tz_min > t_min) t_min = tz_min; 
    if (tz_max < t_max) t_max = tz_max;

	return true;
}

struct Ray
{
	vec3 origin;
	vec3 direction;
};

void ray_generate(out Ray ray)
{
	ray.origin = u_position;
	
	vec2 coord = ivec2(gl_FragCoord.xy) / vec2(u_screen);
	coord = coord * 2.0f - 1.0f;
	
	vec3 d;

	d = normalize(
		mat3(inverse(u_view)) * (inverse(u_projection) * vec4(coord, 1, 1)).xyz
	);
	
	d.x = abs(d.x) > EPS ? d.x : (d.x >= 0 ? EPS : -EPS);
	d.y = abs(d.y) > EPS ? d.y : (d.y >= 0 ? EPS : -EPS);
	d.z = abs(d.z) > EPS ? d.z : (d.z >= 0 ? EPS : -EPS);

	ray.direction = d;
}

struct Stack {
	uint node_address;
	uint frontal_mask;
	float t_min;
	vec3 t_corner;
} stack[MAX_DEPTH];

bool ray_trace(Ray ray, vec3 _min, vec3 _max, out vec4 color, out float t_hit)
{
	float t_min, t_max;
	if (!ray_intersect(ray.origin, ray.direction, _min, _max, t_min, t_max)) {
		return false;
	}

	t_min = max(t_min, 0);

	uint node_address = u_start_address;

	int depth = 0;

	vec3 size = (_max - _min);
	
	uint dir_mask = 0u;
	if (ray.direction.x > 0) dir_mask ^= 1u;
	if (ray.direction.y > 0) dir_mask ^= 2u;
	if (ray.direction.z > 0) dir_mask ^= 4u;
	
	float scale = 0.5;
	vec3 _step = scale * size;

	vec3 center = (_min + _max) / 2;

	vec3 t_center;
	t_center = (center - ray.origin) / ray.direction; // Gets the t values for the center projected along the three axes.

	uint frontal_mask = 0u;
	if (t_center.x > t_min) frontal_mask ^= 1u;
	if (t_center.y > t_min) frontal_mask ^= 2u;
	if (t_center.z > t_min) frontal_mask ^= 4u;
	
	vec3 corner = center;
	if ((frontal_mask & 1u) == 0) corner.x += sign(ray.direction.x) * _step.x;
	if ((frontal_mask & 2u) == 0) corner.y += sign(ray.direction.y) * _step.y;
	if ((frontal_mask & 4u) == 0) corner.z += sign(ray.direction.z) * _step.z;

	uint value = 0;

	vec3 t_corner = (corner - ray.origin) / ray.direction; // Gets the t values for the corner if projected on the ray.

	while (true)
	{
		if (value == 0) {
			value = b_octree[node_address + (frontal_mask ^ dir_mask)];
		}

		if ((value & 0x80000000u) != 0)
		{
			// PUSH
			stack[depth].node_address = node_address;
			stack[depth].frontal_mask = frontal_mask;
			stack[depth].t_min = t_min;
			stack[depth].t_corner = t_corner;

			depth++;
			scale /= 2.0;
			_step = size * scale;
			
			node_address = value & 0x7FFFFFFFu;
			t_center = t_corner - sign(ray.direction) * _step / ray.direction;

			frontal_mask = 0;

			if (t_center.x >= t_min)
			{
				frontal_mask ^= 1u;
				t_corner.x -= sign(ray.direction.x) * _step.x / ray.direction.x;
			}

			if (t_center.y >= t_min)
			{
				frontal_mask ^= 2u;
				t_corner.y -= sign(ray.direction.y) * _step.y / ray.direction.y;
			}

			if (t_center.z >= t_min)
			{
				frontal_mask ^= 4u;
				t_corner.z -= sign(ray.direction.z) * _step.z / ray.direction.z;
			}

			value = 0;
			continue;
		} else if (value > 0) {
			break;
		}

		while (true)
		{
			// ADVANCE
			float t_corner_max = min(min(t_corner.x, t_corner.y), t_corner.z);
			t_min = t_corner_max;

			uint step_mask = 0;

			if (t_corner.x <= t_corner_max)
			{
				step_mask ^= 1u;
				t_corner.x += sign(ray.direction.x) * _step.x / ray.direction.x;
			}

			if (t_corner.y <= t_corner_max)
			{	
				step_mask ^= 2u;
				t_corner.y += sign(ray.direction.y) * _step.y / ray.direction.y;
			}
			
			if (t_corner.z <= t_corner_max)
			{
				step_mask ^= 4u;
				t_corner.z += sign(ray.direction.z) * _step.z / ray.direction.z;
			}

			frontal_mask ^= step_mask;

			if ((frontal_mask & step_mask) == 0) {
				break;
			}

			// POP
			depth--;
			if (depth < 0)
				return false;
			scale *= 2.0;
			_step = size * scale;

			node_address = stack[depth].node_address;
			frontal_mask = stack[depth].frontal_mask;
			t_min = stack[depth].t_min;
			t_corner = stack[depth].t_corner;
		}
		
		value = 0;
	}

	color = octree_unpack_color(value);
	color.rgb /= 255.0;
	color.a /= 127.0;

	t_hit = t_min;

	return true;
}

void main()
{
	Ray ray;
	ray_generate(ray);

	vec4 color;
	float t_hit;
	bool hit = ray_trace(ray, u_octree_from, u_octree_from + u_octree_size, color, t_hit);

	vec3 hit_pos = ray.origin + ray.direction * t_hit;
	vec4 cs_hit_pos = u_projection * u_view * vec4(hit_pos, 1.0); // Hit position in clip space
	gl_FragDepth = (cs_hit_pos.z / cs_hit_pos.w) / 2 + 0.5;
	
	if (!hit)
	{
		discard;
	}

	f_color = color;
}



