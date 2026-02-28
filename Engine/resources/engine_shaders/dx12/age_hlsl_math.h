#pragma once
#include "age_hlsl.h"

static const float sqrt_2	  = 1.41421356f;
static const float sqrt_2_inv = 0.70710678f;

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
	[unroll] for (uint32 i = 0; i < 8; ++i)
	{
		if (i < local_n)
		{
			byte_mask	&= ~(1u << pos_in_byte);
			pos_in_byte	 = firstbitlow(byte_mask);
		}
	}

	return byte_idx * 8u + pos_in_byte;
}