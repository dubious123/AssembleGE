#pragma once

// SDF functions based on work by Inigo Quilez
// https://iquilezles.org/articles
// Licensed under the MIT License
// Copyright (c) Inigo Quilez
#if !defined(AGE_SHADER)
namespace age::inline math
{
	using sdf_shape_data = const uint32 (&)[5];
	#define AGE_SHARED_FUNC(ret, func_name, ...) \
		FORCE_INLINE constexpr ret               \
		func_name(__VA_ARGS__) noexcept

	#define e_fit_mode_kind		   ui::e::fit_mode_kind
	#define UI_FIT_MODE_CONTAIN	   ui::e::fit_mode_kind::contain
	#define UI_FIT_MODE_COVER	   ui::e::fit_mode_kind::cover
	#define UI_FIT_MODE_FILL	   ui::e::fit_mode_kind::fill
	#define UI_FIT_MODE_NONE	   ui::e::fit_mode_kind::none
	#define UI_FIT_MODE_SCALE_DOWN ui::e::fit_mode_kind::scale_down

	static_assert(to_idx(UI_FIT_MODE_CONTAIN) == 0);
	static_assert(to_idx(UI_FIT_MODE_COVER) == 1);
	static_assert(to_idx(UI_FIT_MODE_FILL) == 2);
	static_assert(to_idx(UI_FIT_MODE_NONE) == 3);
	static_assert(to_idx(UI_FIT_MODE_SCALE_DOWN) == 4);
#else
using sdf_shape_data = const uint32[5];
	#define AGE_SHARED_FUNC(ret, func_name, ...) \
		ret func_name(__VA_ARGS__)

	#define e_fit_mode_kind		   uint32
	#define UI_FIT_MODE_CONTAIN	   0
	#define UI_FIT_MODE_COVER	   1
	#define UI_FIT_MODE_FILL	   2
	#define UI_FIT_MODE_NONE	   3
	#define UI_FIT_MODE_SCALE_DOWN 4

#endif


	AGE_SHARED_FUNC(float, sdf_rect, float2 pos, float2 size)
	{
		const float2 delta_from_corner = abs(pos) - size * 0.5;
		const float	 delta_from_edge   = length(max(delta_from_corner, 0.f)) + min(max(delta_from_corner.x, delta_from_corner.y), 0.f);

		return delta_from_edge;
	}

	AGE_SHARED_FUNC(float, sdf_rounded_rect, float2 pos, float2 size, float radius)
	{
		const float2 delta_from_corner = abs(pos) - size * 0.5 + radius;
		const float	 delta_from_edge   = length(max(delta_from_corner, 0.f)) + min(max(delta_from_corner.x, delta_from_corner.y), 0.f);

		return delta_from_edge - radius;
	}

	AGE_SHARED_FUNC(float, sdf_circle, float2 pos, float radius)
	{
		return length(pos) - radius;
	}

	AGE_SHARED_FUNC(float, sdf_ellipse, float2 p, float2 ab)
	{
		p = float2(abs(p.x), abs(p.y));
		if (p.x > p.y)
		{
			p  = float2(p.y, p.x);
			ab = float2(ab.y, ab.x);
		}

		const float l  = ab.y * ab.y - ab.x * ab.x;
		const float m  = ab.x * p.x / l;
		const float m2 = m * m;
		const float n  = ab.y * p.y / l;
		const float n2 = n * n;
		const float c  = (m2 + n2 - 1.f) / 3.f;
		const float c3 = c * c * c;
		const float q  = c3 + m2 * n2 * 2.f;
		const float d  = c3 + m2 * n2;
		const float g  = m + m * n2;

		float co;
		if (d < 0.f)
		{
			const float h  = acos(q / c3) / 3.f;
			const float s  = cos(h);
			const float t  = sin(h) * 1.732050808f;
			const float rx = sqrt(-c * (s + t + 2.f) + m2);
			const float ry = sqrt(-c * (s - t + 2.f) + m2);
			co			   = (ry + sign(l) * rx + abs(g) / (rx * ry) - m) * 0.5f;
		}
		else
		{
			const float h  = 2.f * m * n * sqrt(d);
			const float s  = sign(q + h) * pow(abs(q + h), 1.f / 3.f);
			const float u  = sign(q - h) * pow(abs(q - h), 1.f / 3.f);
			const float rx = -s - u - c * 4.f + 2.f * m2;
			const float ry = (s - u) * 1.732050808f;
			const float rm = sqrt(rx * rx + ry * ry);
			co			   = (ry / sqrt(rm - rx) + 2.f * g / rm - m) * 0.5f;
		}

		const float2 r = float2(ab.x * co, ab.y * sqrt(1.f - co * co));
		return length(r - p) * sign(p.y - r.y);
	}

	AGE_SHARED_FUNC(float, sdf_line, float2 p, float2 a, float2 b)
	{
		const float2 pa = p - a;
		const float2 ba = b - a;
		const float	 h	= clamp(dot(pa, ba) / dot(ba, ba), 0.f, 1.f);
		return length(pa - ba * h);
	}

	AGE_SHARED_FUNC(float, sdf_triangle, float2 pos, float size)
	{
		const float k = sqrt(3.f);
		pos.x		  = abs(pos.x) - size;
		pos.y		  = pos.y + size / k;
		if (pos.x + k * pos.y > 0.f)
		{
			pos = float2(pos.x - k * pos.y, -k * pos.x - pos.y) * 0.5f;
		}
		pos.x -= clamp(pos.x, -2.f * size, 0.f);
		return -length(pos) * sign(pos.y);
	}

	AGE_SHARED_FUNC(float, sdf_round, float delta_from_edge, float round)
	{
		return delta_from_edge - round;
	}

	AGE_SHARED_FUNC(float, sdf_onion, float delta_from_edge, float round)
	{
		return abs(delta_from_edge) - round;
	}

	AGE_SHARED_FUNC(float, sdf_union, float a, float b)
	{
		return min(a, b);
	}

	AGE_SHARED_FUNC(float, sdf_smooth_union, float a, float b, float k)
	{
		const float h = clamp(0.5f + 0.5f * (b - a) / k, 0.f, 1.f);
		return lerp(b, a, h) - k * h * (1.f - h);
	}

	// ui

	AGE_SHARED_FUNC(float, sdf_rect, float2 center_offset, float2 size, e_fit_mode_kind fit_mode, sdf_shape_data data)
	{
		return sdf_rect(center_offset, size);
	}

	AGE_SHARED_FUNC(float, sdf_circle, float2 center_offset, float2 size, e_fit_mode_kind fit_mode, sdf_shape_data data)
	{
		return sdf_circle(center_offset, min(size.x, size.y) * 0.5f);
	}

	AGE_SHARED_FUNC(float, sdf_arrow_right, float2 center_offset, float2 size, e_fit_mode_kind fit_mode, sdf_shape_data data)
	{
		float2 box = size * 0.5 * 0.35f;

		float2 top	  = float2(-box.x * 0.6f, -box.y);
		float2 middle = float2(box.x * 0.6f, 0.f);
		float2 bottom = float2(-box.x * 0.6f, box.y);

		return sdf_union(
				   sdf_line(center_offset, top, middle),
				   sdf_line(center_offset, middle, bottom))
			 - 1.3f;
	}

	AGE_SHARED_FUNC(float, sdf_check, float2 center_offset, float2 size, e_fit_mode_kind fit_mode, sdf_shape_data data)
	{
		float2 box = size * 0.5f * 0.35f;

		float2 start  = float2(-box.x * 0.8f, -box.y * 0.1f);
		float2 corner = float2(-box.x * 0.15f, box.y * 0.7f);
		float2 tip	  = float2(box.x * 0.85f, -box.y * 0.75f);

		return sdf_union(
				   sdf_line(center_offset, start, corner),
				   sdf_line(center_offset, corner, tip))
			 - 1.3f;
	}

	AGE_SHARED_FUNC(float, sdf_rounded_rect, float2 center_offset, float2 size, e_fit_mode_kind fit_mode, sdf_shape_data data)
	{
		float roundness = min(as_float(data[0]), min(size.x, size.y) * 0.5f);
		return sdf_rounded_rect(center_offset, size, roundness);
	}

	AGE_SHARED_FUNC(float, sdf_triangle, float2 center_offset, float2 size, e_fit_mode_kind fit_mode, sdf_shape_data data)
	{
		float s = min(size.x, size.y) * 0.5f;
		return sdf_triangle(center_offset, s);
	}

	AGE_SHARED_FUNC(float, sdf_cross, float2 center_offset, float2 size, e_fit_mode_kind fit_mode, sdf_shape_data data)
	{
		float2 half = size * 0.5f;

		return sdf_union(
				   sdf_line(center_offset, float2(-half.x, -half.y), float2(half.x, half.y)),
				   sdf_line(center_offset, float2(-half.x, half.y), float2(half.x, -half.y)))
			 - 1.3f;
	}

	AGE_SHARED_FUNC(float, sdf_arc, float2 center_offset, float2 size, e_fit_mode_kind fit_mode, sdf_shape_data data)
	{
		const float thickness	 = as_float(data[0]);
		const float aperture_sin = as_float(data[1]);
		const float aperture_cos = as_float(data[2]);

		const float2 p	= float2(abs(center_offset.x), -center_offset.y);
		const float2 sc = float2(aperture_sin, aperture_cos);

		if (fit_mode == UI_FIT_MODE_FILL)
		{
			const float radius_x = size.x * 0.5f - thickness * 0.5f;
			const float radius_y = size.y * 0.5f - thickness * 0.5f;


			const float	 ellipse_dist = sdf_ellipse(p, float2(radius_x, radius_y));
			const float	 arc_dist	  = abs(ellipse_dist) - thickness * 0.5f;
			const float2 endpoint	  = float2(radius_x * sc.x, radius_y * sc.y);
			// const float2 tangent	  = float2(-radius_x * sc.y, radius_y * sc.x);
			const float2 normal = float2(radius_y * sc.x, radius_x * sc.y);
			const float2 e_to_p = p - endpoint;

			const float cos_theta = dot(normal, e_to_p) / (length(normal) * length(e_to_p));

			const float cross_z = normal.x * e_to_p.y - normal.y * e_to_p.x;

			bool inside_arc = false;

			if (sc.y > 0.f)
			{
				inside_arc = p.y > 0 and cross_z > 0.f and (abs(cos_theta) < 1.f - 0.1f);
				// inside_arc = p.y > 0 and (abs(cos_theta) < 1.f - 0.1f);
			}
			else
			{
				inside_arc = p.y > 0 or (p.y < 0.f and cross_z > 0.f and (abs(cos_theta) < 1.f - 0.1f));
			}

			if (inside_arc)
			{
				return abs(arc_dist) <= thickness ? arc_dist : 2.f;
			}
			else
			{
				return 2.f;

				const float cap_dist = length(p - endpoint) - thickness * 0.5f;
				return sdf_union(arc_dist, cap_dist);
			}
		}

		const float radius = min(size.x, size.y) * 0.5f - thickness * 0.5f;

		return ((sc.y * p.x > sc.x * p.y)
					? length(p - sc * radius)
					: abs(length(p) - radius))
			 - thickness * 0.5f;
	}

#if !defined(AGE_SHADER)
	#undef AGE_SHARED_FUNC
	#undef e_fit_mode_kind
	#undef UI_FIT_MODE_CONTAIN
	#undef UI_FIT_MODE_COVER
	#undef UI_FIT_MODE_FILL
	#undef UI_FIT_MODE_NONE
	#undef UI_FIT_MODE_SCALE_DOWN
}
#endif