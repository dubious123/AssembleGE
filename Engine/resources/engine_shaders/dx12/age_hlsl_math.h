#pragma once

static const float epsilon_1e4 = 0.0001f;
static const float epsilon_1e6 = 0.000001f;
static const float sqrt_2	   = 1.41421356f;
static const float sqrt_2_inv  = 0.70710678f;
static const float pi_half	   = 1.570796327f;

static const uint32 invalid_id_uint32 = 0xffffffff;

float
int8_to_float(uint32 raw)
{
	return float(int((raw & 0xffu) << 24) >> 24);
}

float
snorm8_to_float(int32 raw)
{
	return ((raw << 24) >> 24) / 127.f;
}

float
unorm8_to_float(uint32 raw)
{
	return (raw & 0xffu) / 255.f;
}

int32
uint32_x_to_int8(uint32 u)
{
	return (int32)(int32(u << 24) >> 24);
}

int32
uint32_y_to_int8(uint32 u)
{
	return (int32)(int32(u << 16) >> 24);
}

int32
uint32_z_to_int8(uint32 u)
{
	return (int32)(int32(u << 8) >> 24);
}

int32
uint32_w_to_int8(uint32 u)
{
	return (int32)(int32(u) >> 24);
}

int16
uint32_upper_to_int16(uint32 u)
{
	return int16(u >> 16);
}

int16
uint32_lower_to_int16(uint32 u)
{
	return int16(u & 0xffff);
}

uint16
uint32_upper_to_uint16(uint32 u)
{
	return uint16(u >> 16);
}

uint16
uint32_lower_to_uint16(uint32 u)
{
	return uint16(u & 0xffff);
}

half
uint32_lower_to_half(uint32 u)
{
	return asfloat16(uint32_lower_to_uint16(u));
}

half
uint32_upper_to_half(uint32 u)
{
	return asfloat16(uint32_upper_to_uint16(u));
}

half2
uint32_to_half2(uint32 u)
{
	return half2(uint32_lower_to_half(u), uint32_upper_to_half(u));
}

// 10-10-10-2 Smallest Three Quaternion decoding
float4
decode_quaternion(uint32 encoded_q)
{
	uint32 index = (encoded_q >> 30) & 0x3;
	float3 c	 = float3(
		(float)((encoded_q >> 0) & 0x3ff),		// x
		(float)((encoded_q >> 10) & 0x3ff),		// y
		(float)((encoded_q >> 20) & 0x3ff));	// z

	c = c / 1023.f * sqrt_2 - sqrt_2_inv;

	float missing = sqrt(saturate(1.f - dot(c, c)));

	switch (index)
	{
	case 0:
		return float4(missing, c.x, c.y, c.z);
	case 1:
		return float4(c.x, missing, c.y, c.z);
	case 2:
		return float4(c.x, c.y, missing, c.z);
	default:	// case 3
		return float4(c.x, c.y, c.z, missing);
	}
}

float3
rotate(float3 v, float4 q)
{
	// v' = v + 2*cross(q.xyz, cross(q.xyz, v) + q.w*v)
	float3 t = 2.0f * cross(q.xyz, v);
	return v + q.w * t + cross(q.xyz, t);
}

float3
decode_oct_snorm(const uint16 oct)
{
	const float x = snorm8_to_float(oct & 0xffu);
	const float y = snorm8_to_float((oct >> 8) & 0xffu);

	float3 res;
	res.xy = float2(x, y);
	res.z  = 1.f - abs(res.x) - abs(res.y);

	if (res.z < 0.f)
	{
		const float old_x = res.x;
		const float old_y = res.y;

		res.x = (1.f - abs(old_y)) * (old_x >= 0.f ? 1.f : -1.f);
		res.y = (1.f - abs(old_x)) * (old_y >= 0.f ? 1.f : -1.f);
	}

	return res;
}

float3
decode_oct_unorm(const uint16 oct)
{
	const float x = unorm8_to_float(oct & 0xffu);
	const float y = unorm8_to_float((oct >> 8) & 0xffu);

	float3 res;
	res.xy = float2(x, y) * 2.f - 1.f;
	res.z  = 1.f - abs(res.x) - abs(res.y);

	if (res.z < 0.f)
	{
		const float old_x = res.x;
		const float old_y = res.y;

		res.x = (1.f - abs(old_y)) * (old_x >= 0.f ? 1.f : -1.f);
		res.y = (1.f - abs(old_x)) * (old_y >= 0.f ? 1.f : -1.f);
	}

	return res;
}

// precondition:
//  - mask != 0
//  - n < countbits(mask)   (n은 0-based: 0번째 set bit, 1번째 set bit...)
// returns:
// if mask = 1110, n = 2 => 1000 => 3
// if mask = 10100, n = 0 => 100 => 2
uint32
select32_nth_set_bit(const uint32 mask, const uint32 n)
{
	uint32 b0 = (mask) & 0xFFu;
	uint32 b1 = (mask >> 8) & 0xFFu;
	uint32 b2 = (mask >> 16) & 0xFFu;
	uint32 b3 = (mask >> 24) & 0xFFu;

	uint32 c0 = countbits(b0);
	uint32 c1 = countbits(b1);
	uint32 c2 = countbits(b2);
	uint32 c3 = countbits(b3);

	// uint32 p0 = 0;
	uint32 p1 = c0;
	uint32 p2 = c0 + c1;
	uint32 p3 = c0 + c1 + c2;

	uint32 byte_idx = 0 + (n >= p1) + (n >= p2) + (n >= p3);

	uint32 p_arr[4] = { 0u, p1, p2, p3 };
	uint32 prefix	= p_arr[byte_idx];

	uint32 local_n = n - prefix;

	// byte_mask cannot be 0 because n < countbits(mask)
	uint32 byte_mask = (mask >> (byte_idx * 8u)) & 0xFFu;

	uint32 pos_in_byte = firstbitlow(byte_mask);
	UNROLL
	for (uint32 i = 0; i < 8; ++i)
	{
		if (i < local_n)
		{
			byte_mask	&= ~(1u << pos_in_byte);
			pos_in_byte	 = firstbitlow(byte_mask);
		}
	}

	return byte_idx * 8u + pos_in_byte;
}

uint32
float_to_sortable(float f)
{
	uint32 bits = asuint(f);
	uint32 mask = (bits & 0x80000000) ? 0xFFFFFFFF : 0x80000000;
	return bits ^ mask;
}

float2
screen_to_ndc(float2 screen_pos, float2 inv_backbuffer_size)
{
	float2 ndc = screen_pos * inv_backbuffer_size * 2.0 - 1.0;
	ndc.y	   = -ndc.y;
	return ndc;
}

float2
ndc_xy_to_screen(float2 ndc_xy, float2 screen_size)
{
	return float2(ndc_xy.x * 0.5 + 0.5, -ndc_xy.y * 0.5 + 0.5) * screen_size;
}

bool
sphere_aabb_intersect(float3 sphere_position, float sphere_range, float3 aabb_min, float3 aabb_max)
{
	const float3 closest = clamp(sphere_position, aabb_min, aabb_max);
	const float3 diff	 = sphere_position - closest;
	return dot(diff, diff) <= sphere_range * sphere_range;
}

bool
sphere_frustum_test(float3 center, float radius, float4 frustum_planes[6])
{
	UNROLL
	for (uint32 i = 0; i < 6; i++)
	{
		if (dot(frustum_planes[i], float4(center, 1.f)) < -radius)
		{
			return false;
		}
	}
	return true;
}

float
calc_point_to_plane_distance(float3 pos, float4 plane)
{
	return dot(pos, plane.xyz) + plane.w;
}

float
linearize_reverse_z(float z, float n, float f)
{
	return n * f / (z * (f - n) + n);
}

float4x4
view_look_to(float3 pos, float3 dir)
{
	const float3 z_axis = normalize(dir);
	const float3 up		= abs(dot(z_axis, float3(0, 1, 0))) > 0.999f
							? float3(1, 0, 0)
							: float3(0, 1, 0);
	const float3 x_axis = normalize(cross(up, z_axis));
	const float3 y_axis = cross(z_axis, x_axis);

	return float4x4(
		x_axis.x, x_axis.y, x_axis.z, -dot(x_axis, pos),
		y_axis.x, y_axis.y, y_axis.z, -dot(y_axis, pos),
		z_axis.x, z_axis.y, z_axis.z, -dot(z_axis, pos),
		0, 0, 0, 1);
}

float4x4
translation(float x, float y, float z)
{
	return float4x4(
		1, 0, 0, x,
		0, 1, 0, y,
		0, 0, 1, z,
		0, 0, 0, 1);
}

float4x4
proj_perspective_reversed(float fov_y, float aspect_ratio, float near_z, float far_z)
{
	float h = 1.0f / tan(fov_y * 0.5f);
	float w = h / aspect_ratio;
	float a = near_z / (far_z - near_z);
	float b = far_z * near_z / (far_z - near_z);

	return float4x4(
		w, 0.0f, 0.0f, 0.0f,
		0.0f, h, 0.0f, 0.0f,
		0.0f, 0.0f, a, 1.0f,
		0.0f, 0.0f, b, 0.0f);
}

float4x4
proj_orthographic_reversed(float width, float height, float near_z, float far_z)
{
	const float rng = far_z - near_z;

	return float4x4(
		2.0 / width, 0, 0, 0,
		0, 2.0 / height, 0, 0,
		0, 0, -1.0 / rng, far_z / rng,
		0, 0, 0, 1);
}

float4
normalize_plane(float4 p)
{
	return p / length(p.xyz);
}

void
gen_frustum_planes(float4x4 view_proj_mat, out float4 planes[6])
{
	// frustum planes (Gribb-Hartmann)
	const float4 r0 = view_proj_mat[0];
	const float4 r1 = view_proj_mat[1];
	const float4 r2 = view_proj_mat[2];
	const float4 r3 = view_proj_mat[3];

	planes[0] = normalize_plane(r3 + r0);	 // left
	planes[1] = normalize_plane(r3 - r0);	 // right
	planes[2] = normalize_plane(r3 - r1);	 // top
	planes[3] = normalize_plane(r3 + r1);	 // bottom
	planes[4] = normalize_plane(r2);		 // near
	planes[5] = normalize_plane(r3 - r2);	 // far
}
