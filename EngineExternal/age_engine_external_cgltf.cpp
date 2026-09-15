// age::external::cgltf boundary implementation. see cgltf_boundary.hpp for the contract.

#define _CRT_SECURE_NO_WARNINGS
#include "age_engine_external_libs.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4996)
#endif
#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

namespace age::external::cgltf
{
	using namespace detail;

	namespace
	{
		// ---------------------------------------------------------------
		// context
		// ---------------------------------------------------------------

		struct ctx
		{
			const cgltf_data* p_src;
			int*			  p_view_to_block;	  // buffer_view idx -> meshopt block idx, -1 = none
		};

		// ---------------------------------------------------------------
		// generic helpers
		// ---------------------------------------------------------------

		template <typename T>
		T*
		alloc_arr(cgltf_size n)
		{
			return n ? new T[n]() : nullptr;
		}

		template <typename T>
		void
		free_arr(T*& p)
		{
			delete[] p;
			p = nullptr;
		}

		template <typename T>
		int
		idx_of(const T* p, const T* base)
		{
			return p ? static_cast<int>(p - base) : -1;
		}

		name_view
		make_name(const char* s)
		{
			return s ? name_view{ s, static_cast<int>(std::strlen(s)) } : name_view{ nullptr, 0 };
		}

		name_view
		cvt_extras(const cgltf_extras& e)
		{
			return make_name(e.data);
		}

		void
		fill_extensions(const cgltf_extension* p, cgltf_size n, extension_data*& out, int& out_count)
		{
			out		  = alloc_arr<extension_data>(n);
			out_count = static_cast<int>(n);
			for (cgltf_size i = 0; i < n; ++i)
			{
				out[i].name = make_name(p[i].name);
				out[i].json = make_name(p[i].data);
			}
		}

		void
		fill_names(char* const* p, cgltf_size n, name_view*& out, int& out_count)
		{
			out		  = alloc_arr<name_view>(n);
			out_count = static_cast<int>(n);
			for (cgltf_size i = 0; i < n; ++i)
			{
				out[i] = make_name(p[i]);
			}
		}

		// ---------------------------------------------------------------
		// enum conversion
		// ---------------------------------------------------------------

		load_error
		cvt_error(cgltf_result r)
		{
			switch (r)
			{
			case cgltf_result_success:
				return load_error::none;
			case cgltf_result_file_not_found:
			case cgltf_result_io_error:
				return load_error::buffer_not_found;
			case cgltf_result_out_of_memory:
			case cgltf_result_invalid_options:
				return load_error::internal;
			default:
				return load_error::invalid_format;
			}
		}

		primitive_mode
		cvt_primitive_mode(cgltf_primitive_type t)
		{
			switch (t)
			{
			case cgltf_primitive_type_points:
				return primitive_mode::points;
			case cgltf_primitive_type_lines:
				return primitive_mode::lines;
			case cgltf_primitive_type_line_loop:
				return primitive_mode::line_loop;
			case cgltf_primitive_type_line_strip:
				return primitive_mode::line_strip;
			case cgltf_primitive_type_triangle_strip:
				return primitive_mode::triangle_strip;
			case cgltf_primitive_type_triangle_fan:
				return primitive_mode::triangle_fan;
			default:
				return primitive_mode::triangles;	 // glTF default
			}
		}

		animation_path
		cvt_animation_path(cgltf_animation_path_type t)
		{
			switch (t)
			{
			case cgltf_animation_path_type_rotation:
				return animation_path::rotation;
			case cgltf_animation_path_type_scale:
				return animation_path::scale;
			case cgltf_animation_path_type_weights:
				return animation_path::weights;
			default:
				return animation_path::translation;
			}
		}

		interpolation_kind
		cvt_interpolation(cgltf_interpolation_type t)
		{
			switch (t)
			{
			case cgltf_interpolation_type_step:
				return interpolation_kind::step;
			case cgltf_interpolation_type_cubic_spline:
				return interpolation_kind::cubic_spline;
			default:
				return interpolation_kind::linear;
			}
		}

		light_kind
		cvt_light_kind(cgltf_light_type t)
		{
			switch (t)
			{
			case cgltf_light_type_point:
				return light_kind::point;
			case cgltf_light_type_spot:
				return light_kind::spot;
			default:
				return light_kind::directional;
			}
		}

		camera_kind
		cvt_camera_kind(cgltf_camera_type t)
		{
			return t == cgltf_camera_type_orthographic ? camera_kind::orthographic : camera_kind::perspective;
		}

		alpha_kind
		cvt_alpha(cgltf_alpha_mode m)
		{
			switch (m)
			{
			case cgltf_alpha_mode_mask:
				return alpha_kind::mask;
			case cgltf_alpha_mode_blend:
				return alpha_kind::blend;
			default:
				return alpha_kind::opaque;
			}
		}

		wrap_kind
		cvt_wrap(cgltf_wrap_mode w)
		{
			switch (w)
			{
			case cgltf_wrap_mode_clamp_to_edge:
				return wrap_kind::clamp_to_edge;
			case cgltf_wrap_mode_mirrored_repeat:
				return wrap_kind::mirrored_repeat;
			default:
				return wrap_kind::repeat;
			}
		}

		filter_kind
		cvt_mag_filter(cgltf_filter_type f)
		{
			switch (f)
			{
			case cgltf_filter_type_nearest:
				return filter_kind::nearest;
			case cgltf_filter_type_linear:
				return filter_kind::linear;
			default:
				return filter_kind::undefined;
			}
		}

		void
		cvt_min_filter(cgltf_filter_type f, filter_kind& out_min, mipmap_kind& out_mip)
		{
			switch (f)
			{
			case cgltf_filter_type_nearest:
				out_min = filter_kind::nearest;
				out_mip = mipmap_kind::none;
				break;
			case cgltf_filter_type_linear:
				out_min = filter_kind::linear;
				out_mip = mipmap_kind::none;
				break;
			case cgltf_filter_type_nearest_mipmap_nearest:
				out_min = filter_kind::nearest;
				out_mip = mipmap_kind::nearest;
				break;
			case cgltf_filter_type_linear_mipmap_nearest:
				out_min = filter_kind::linear;
				out_mip = mipmap_kind::nearest;
				break;
			case cgltf_filter_type_nearest_mipmap_linear:
				out_min = filter_kind::nearest;
				out_mip = mipmap_kind::linear;
				break;
			case cgltf_filter_type_linear_mipmap_linear:
				out_min = filter_kind::linear;
				out_mip = mipmap_kind::linear;
				break;
			default:
				out_min = filter_kind::undefined;
				out_mip = mipmap_kind::none;
				break;
			}
		}

		component_kind
		cvt_component(cgltf_component_type t)
		{
			switch (t)
			{
			case cgltf_component_type_r_8:
				return component_kind::i8;
			case cgltf_component_type_r_8u:
				return component_kind::u8;
			case cgltf_component_type_r_16:
				return component_kind::i16;
			case cgltf_component_type_r_16u:
				return component_kind::u16;
			case cgltf_component_type_r_32u:
				return component_kind::u32;
			default:
				return component_kind::f32;
			}
		}

		meshopt_mode
		cvt_meshopt_mode(cgltf_meshopt_compression_mode m)
		{
			switch (m)
			{
			case cgltf_meshopt_compression_mode_triangles:
				return meshopt_mode::triangles;
			case cgltf_meshopt_compression_mode_indices:
				return meshopt_mode::indices;
			default:
				return meshopt_mode::attributes;
			}
		}

		meshopt_filter
		cvt_meshopt_filter(cgltf_meshopt_compression_filter f)
		{
			switch (f)
			{
			case cgltf_meshopt_compression_filter_octahedral:
				return meshopt_filter::octahedral;
			case cgltf_meshopt_compression_filter_quaternion:
				return meshopt_filter::quaternion;
			case cgltf_meshopt_compression_filter_exponential:
				return meshopt_filter::exponential;
			default:
				return meshopt_filter::none;
			}
		}

		unsigned int
		cvt_feature_mask(const cgltf_material& m)
		{
			unsigned int mask = 0;
			if (m.unlit) mask |= material_feature_unlit;
			if (m.has_pbr_specular_glossiness) mask |= material_feature_specular_glossiness;
			if (m.has_clearcoat) mask |= material_feature_clearcoat;
			if (m.has_transmission) mask |= material_feature_transmission;
			if (m.has_volume) mask |= material_feature_volume;
			if (m.has_ior) mask |= material_feature_ior;
			if (m.has_specular) mask |= material_feature_specular;
			if (m.has_sheen) mask |= material_feature_sheen;
			if (m.has_emissive_strength) mask |= material_feature_emissive_strength;
			if (m.has_iridescence) mask |= material_feature_iridescence;
			if (m.has_diffuse_transmission) mask |= material_feature_diffuse_transmission;
			if (m.has_anisotropy) mask |= material_feature_anisotropy;
			if (m.has_dispersion) mask |= material_feature_dispersion;
			return mask;
		}

		// ---------------------------------------------------------------
		// matrices. cgltf gives column-major (m[col * 4 + row]), engine
		// wants row-major (m[row * 4 + col]) -> transpose.
		// ---------------------------------------------------------------

		void
		transpose_16(const float* in, float* out)
		{
			for (int r = 0; r < 4; ++r)
			{
				for (int c = 0; c < 4; ++c)
				{
					out[r * 4 + c] = in[c * 4 + r];
				}
			}
		}

		void
		transpose_16_in_place(float* m)
		{
			float t[16];
			transpose_16(m, t);
			std::memcpy(m, t, sizeof(t));
		}

		// decompose a column-major affine matrix into translation * rotation * scale.
		// translation and scale are always valid. returns true when the rotation
		// could not be recovered (shear or zero-length axis), rotation is then unreliable.
		bool
		decompose_trs(const float* m, trs_data& out)
		{
			out.translation[0] = m[12];
			out.translation[1] = m[13];
			out.translation[2] = m[14];

			float c[3][3];	  // c[i] = i-th column (basis axis)
			for (int i = 0; i < 3; ++i)
			{
				c[i][0] = m[i * 4 + 0];
				c[i][1] = m[i * 4 + 1];
				c[i][2] = m[i * 4 + 2];
			}

			float det = c[0][0] * (c[1][1] * c[2][2] - c[2][1] * c[1][2])
					  - c[1][0] * (c[0][1] * c[2][2] - c[2][1] * c[0][2])
					  + c[2][0] * (c[0][1] * c[1][2] - c[1][1] * c[0][2]);

			for (int i = 0; i < 3; ++i)
			{
				out.scale[i] = std::sqrt(c[i][0] * c[i][0] + c[i][1] * c[i][1] + c[i][2] * c[i][2]);
			}
			if (det < 0.f)
			{
				out.scale[0] = -out.scale[0];
			}

			bool decompose_trs_failed = false;
			for (int i = 0; i < 3; ++i)
			{
				float s = out.scale[i];
				if (std::fabs(s) < std::numeric_limits<float>::min())
				{
					// zero-length axis, cannot normalize. zero the column so the
					// quaternion path stays finite, the result is flagged below.
					c[i][0] = c[i][1] = c[i][2] = 0.f;
					decompose_trs_failed		= true;
					continue;
				}
				c[i][0] /= s;
				c[i][1] /= s;
				c[i][2] /= s;
			}

			const float shear_eps = 1e-4f;
			for (int i = 0; i < 3; ++i)
			{
				int	  j = (i + 1) % 3;
				float d = c[i][0] * c[j][0] + c[i][1] * c[j][1] + c[i][2] * c[j][2];
				if (std::fabs(d) > shear_eps)
				{
					decompose_trs_failed = true;
				}
			}

			// rotation matrix R[row][col] = c[col][row], to quaternion
			float r00 = c[0][0], r01 = c[1][0], r02 = c[2][0];
			float r10 = c[0][1], r11 = c[1][1], r12 = c[2][1];
			float r20 = c[0][2], r21 = c[1][2], r22 = c[2][2];

			float trace = r00 + r11 + r22;
			float q[4];	   // xyzw
			if (trace > 0.f)
			{
				float s = std::sqrt(trace + 1.f) * 2.f;
				q[3]	= 0.25f * s;
				q[0]	= (r21 - r12) / s;
				q[1]	= (r02 - r20) / s;
				q[2]	= (r10 - r01) / s;
			}
			else if (r00 > r11 and r00 > r22)
			{
				float s = std::sqrt(1.f + r00 - r11 - r22) * 2.f;
				q[3]	= (r21 - r12) / s;
				q[0]	= 0.25f * s;
				q[1]	= (r01 + r10) / s;
				q[2]	= (r02 + r20) / s;
			}
			else if (r11 > r22)
			{
				float s = std::sqrt(1.f + r11 - r00 - r22) * 2.f;
				q[3]	= (r02 - r20) / s;
				q[0]	= (r01 + r10) / s;
				q[1]	= 0.25f * s;
				q[2]	= (r12 + r21) / s;
			}
			else
			{
				float s = std::sqrt(1.f + r22 - r00 - r11) * 2.f;
				q[3]	= (r10 - r01) / s;
				q[0]	= (r02 + r20) / s;
				q[1]	= (r12 + r21) / s;
				q[2]	= 0.25f * s;
			}

			float len = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
			if (len > 1e-12f)
			{
				for (float& v : q)
					v /= len;
			}
			else
			{
				q[0] = q[1] = q[2]	 = 0.f;
				q[3]				 = 1.f;
				decompose_trs_failed = true;
			}
			std::memcpy(out.rotation, q, sizeof(q));
			return decompose_trs_failed;
		}

		// ---------------------------------------------------------------
		// accessors
		// ---------------------------------------------------------------

		bool
		has_readable_base(const cgltf_accessor* acc)
		{
			// no buffer view and not sparse = draco-only, unpack would zero-fill
			return acc && (acc->buffer_view != nullptr || acc->is_sparse);
		}

		meshopt_ref
		make_meshopt_ref(const cgltf_accessor* acc, const ctx& c)
		{
			meshopt_ref r{};
			r.block_idx = -1;
			if (!acc || !acc->buffer_view || !acc->buffer_view->has_meshopt_compression)
			{
				return r;
			}
			r.block_idx	  = c.p_view_to_block[acc->buffer_view - c.p_src->buffer_views];
			r.byte_offset = static_cast<int>(acc->offset);
			r.count		  = static_cast<int>(acc->count);
			r.component	  = cvt_component(acc->component_type);
			r.components  = static_cast<int>(cgltf_num_components(acc->type));
			r.normalized  = acc->normalized != 0;
			return r;
		}

		float_view
		unpack_floats(const cgltf_accessor* acc)
		{
			if (!has_readable_base(acc))
			{
				return {};
			}
			cgltf_size n = cgltf_accessor_unpack_floats(acc, nullptr, 0);
			if (n == 0)
			{
				return {};
			}
			float* p = new float[n];
			if (cgltf_accessor_unpack_floats(acc, p, n) != n)
			{
				delete[] p;	   // no decoded bytes (meshopt without fallback)
				return {};
			}
			return { p, static_cast<int>(n) };
		}

		index_view
		unpack_uints(const cgltf_accessor* acc)
		{
			if (!has_readable_base(acc))
			{
				return {};
			}
			cgltf_size comps = cgltf_num_components(acc->type);
			cgltf_size n	 = acc->count * comps;
			if (n == 0)
			{
				return {};
			}
			unsigned int* p	 = new unsigned int[n];
			bool		  ok = true;
			if (acc->is_sparse)
			{
				// read_uint / unpack_indices reject sparse. go through floats.
				float* f = new float[n];
				ok		 = cgltf_accessor_unpack_floats(acc, f, n) == n;
				for (cgltf_size i = 0; ok && i < n; ++i)
				{
					p[i] = static_cast<unsigned int>(f[i]);
				}
				delete[] f;
			}
			else if (comps == 1)
			{
				ok = cgltf_accessor_unpack_indices(acc, p, sizeof(unsigned int), acc->count) == acc->count;
			}
			else
			{
				for (cgltf_size i = 0; ok && i < acc->count; ++i)
				{
					ok = cgltf_accessor_read_uint(acc, i, p + i * comps, comps) != 0;
				}
			}
			if (!ok)
			{
				delete[] p;
				return {};
			}
			return { p, static_cast<int>(n) };
		}

		attribute_view
		make_attribute(const cgltf_accessor* acc, const ctx& c)
		{
			attribute_view v{};
			v.view	  = unpack_floats(acc);
			v.meshopt = make_meshopt_ref(acc, c);
			return v;
		}

		index_attribute_view
		make_index_attribute(const cgltf_accessor* acc, const ctx& c)
		{
			index_attribute_view v{};
			v.view	  = unpack_uints(acc);
			v.meshopt = make_meshopt_ref(acc, c);
			return v;
		}

		// engine convention only reachable for the dense view
		attribute_view
		make_matrix_attribute(const cgltf_accessor* acc, const ctx& c)
		{
			attribute_view v = make_attribute(acc, c);
			for (int i = 0; i + 16 <= v.view.count; i += 16)
			{
				transpose_16_in_place(const_cast<float*>(v.view.p) + i);
			}
			return v;
		}

		void
		free_attribute(attribute_view& v)
		{
			delete[] v.view.p;
			v.view = {};
		}

		void
		free_index_attribute(index_attribute_view& v)
		{
			delete[] v.view.p;
			v.view = {};
		}

		// ---------------------------------------------------------------
		// attribute lists (primitive, morph target, instancing)
		// ---------------------------------------------------------------

		const cgltf_accessor*
		find_attribute(const cgltf_attribute* p, cgltf_size n, cgltf_attribute_type t, int set)
		{
			for (cgltf_size i = 0; i < n; ++i)
			{
				if (p[i].type == t && p[i].index == set)
				{
					return p[i].data;
				}
			}
			return nullptr;
		}

		const cgltf_attribute*
		find_attribute_by_name(const cgltf_attribute* p, cgltf_size n, const char* name)
		{
			for (cgltf_size i = 0; i < n; ++i)
			{
				if (p[i].name && std::strcmp(p[i].name, name) == 0)
				{
					return &p[i];
				}
			}
			return nullptr;
		}

		int
		set_count(const cgltf_attribute* p, cgltf_size n, cgltf_attribute_type t)
		{
			int max_idx = -1;
			for (cgltf_size i = 0; i < n; ++i)
			{
				if (p[i].type == t && p[i].index > max_idx)
				{
					max_idx = p[i].index;
				}
			}
			return max_idx + 1;
		}

		bool
		is_custom_attribute(const cgltf_attribute& a)
		{
			return a.type == cgltf_attribute_type_custom || a.type == cgltf_attribute_type_invalid;
		}

		void
		fill_attribute_set(const cgltf_attribute* p, cgltf_size n, cgltf_attribute_type t,
						   const ctx& c, attribute_view*& out, int& out_count)
		{
			out_count = set_count(p, n, t);
			out		  = alloc_arr<attribute_view>(out_count);
			for (int s = 0; s < out_count; ++s)
			{
				out[s] = make_attribute(find_attribute(p, n, t, s), c);
			}
		}

		void
		fill_components_set(const cgltf_attribute* p, cgltf_size n, cgltf_attribute_type t,
							int count, int*& out)
		{
			out = alloc_arr<int>(count);
			for (int s = 0; s < count; ++s)
			{
				const cgltf_accessor* acc = find_attribute(p, n, t, s);
				out[s]					  = acc ? static_cast<int>(cgltf_num_components(acc->type)) : 0;
			}
		}

		void
		fill_custom_set(const cgltf_attribute* p, cgltf_size n, const ctx& c,
						custom_attribute_data*& out, int& out_count, const char* const* p_exclude, int exclude_count)
		{
			cgltf_size cnt = 0;
			for (cgltf_size i = 0; i < n; ++i)
			{
				if (is_custom_attribute(p[i]))
				{
					bool excluded = false;
					for (int e = 0; e < exclude_count; ++e)
					{
						if (p[i].name && std::strcmp(p[i].name, p_exclude[e]) == 0) excluded = true;
					}
					if (!excluded) ++cnt;
				}
			}
			out			 = alloc_arr<custom_attribute_data>(cnt);
			out_count	 = static_cast<int>(cnt);
			cgltf_size k = 0;
			for (cgltf_size i = 0; i < n; ++i)
			{
				if (!is_custom_attribute(p[i])) continue;
				bool excluded = false;
				for (int e = 0; e < exclude_count; ++e)
				{
					if (p[i].name && std::strcmp(p[i].name, p_exclude[e]) == 0) excluded = true;
				}
				if (excluded) continue;
				out[k].name		  = make_name(p[i].name);
				out[k].data		  = make_attribute(p[i].data, c);
				out[k].components = p[i].data ? static_cast<int>(cgltf_num_components(p[i].data->type)) : 0;
				++k;
			}
		}

		void
		free_custom_set(custom_attribute_data*& p, int n)
		{
			for (int i = 0; i < n; ++i)
			{
				free_attribute(p[i].data);
			}
			free_arr(p);
		}

		// ---------------------------------------------------------------
		// mesh
		// ---------------------------------------------------------------

		void
		fill_morph_target(const cgltf_morph_target& src, const ctx& c, morph_target_data& out)
		{
			const cgltf_attribute* p = src.attributes;
			cgltf_size			   n = src.attributes_count;

			out.position_delta = make_attribute(find_attribute(p, n, cgltf_attribute_type_position, 0), c);
			out.normal_delta   = make_attribute(find_attribute(p, n, cgltf_attribute_type_normal, 0), c);
			out.tangent_delta  = make_attribute(find_attribute(p, n, cgltf_attribute_type_tangent, 0), c);

			fill_attribute_set(p, n, cgltf_attribute_type_texcoord, c, out.p_uv_delta, out.uv_delta_count);
			fill_attribute_set(p, n, cgltf_attribute_type_color, c, out.p_color_delta, out.color_delta_count);
			fill_components_set(p, n, cgltf_attribute_type_color, out.color_delta_count, out.p_color_delta_components);
			fill_custom_set(p, n, c, out.p_custom, out.custom_count, nullptr, 0);
		}

		void
		free_morph_target(morph_target_data& t)
		{
			free_attribute(t.position_delta);
			free_attribute(t.normal_delta);
			free_attribute(t.tangent_delta);
			for (int i = 0; i < t.uv_delta_count; ++i)
				free_attribute(t.p_uv_delta[i]);
			free_arr(t.p_uv_delta);
			for (int i = 0; i < t.color_delta_count; ++i)
				free_attribute(t.p_color_delta[i]);
			free_arr(t.p_color_delta);
			free_arr(t.p_color_delta_components);
			free_custom_set(t.p_custom, t.custom_count);
		}

		void
		fill_draco(const cgltf_primitive& prim, const ctx& c, submesh_data& out)
		{
			out.draco_position_id  = -1;
			out.draco_normal_id	   = -1;
			out.draco_tangent_id   = -1;
			out.p_draco_uv_id	   = alloc_arr<int>(out.uv_count);
			out.p_draco_color_id   = alloc_arr<int>(out.color_count);
			out.p_draco_joints_id  = alloc_arr<int>(out.joints_count);
			out.p_draco_weights_id = alloc_arr<int>(out.joints_count);
			out.p_draco_custom_id  = alloc_arr<int>(out.custom_count);
			for (int i = 0; i < out.uv_count; ++i) out
				.p_draco_uv_id[i] = -1;
			for (int i = 0; i < out.color_count; ++i) out
				.p_draco_color_id[i] = -1;
			for (int i = 0; i < out.joints_count; ++i) out
				.p_draco_joints_id[i] = out.p_draco_weights_id[i] = -1;
			for (int i = 0; i < out.custom_count; ++i) out
				.p_draco_custom_id[i] = -1;

			if (!prim.has_draco_mesh_compression)
			{
				return;
			}
			const cgltf_draco_mesh_compression& d	  = prim.draco_mesh_compression;
			const cgltf_buffer_view*			bv	  = d.buffer_view;
			const uint8_t*						bytes = bv ? cgltf_buffer_view_data(bv) : nullptr;
			out.p_draco								  = bytes;
			out.draco_size							  = bytes ? static_cast<int>(bv->size) : 0;

			for (cgltf_size i = 0; i < d.attributes_count; ++i)
			{
				const cgltf_attribute& a  = d.attributes[i];
				int					   id = idx_of(a.data, c.p_src->accessors);	   // cgltf fixed the id up as an accessor pointer
				switch (a.type)
				{
				case cgltf_attribute_type_position:
					out.draco_position_id = id;
					break;
				case cgltf_attribute_type_normal:
					out.draco_normal_id = id;
					break;
				case cgltf_attribute_type_tangent:
					out.draco_tangent_id = id;
					break;
				case cgltf_attribute_type_texcoord:
					if (a.index < out.uv_count) out
						.p_draco_uv_id[a.index] = id;
					break;
				case cgltf_attribute_type_color:
					if (a.index < out.color_count) out
						.p_draco_color_id[a.index] = id;
					break;
				case cgltf_attribute_type_joints:
					if (a.index < out.joints_count) out
						.p_draco_joints_id[a.index] = id;
					break;
				case cgltf_attribute_type_weights:
					if (a.index < out.joints_count) out
						.p_draco_weights_id[a.index] = id;
					break;
				default:
					for (int k = 0; k < out.custom_count; ++k)
					{
						if (a.name&& out.p_custom[k].name.p && std::strcmp(a.name, out.p_custom[k].name.p) == 0)
						{
							out.p_draco_custom_id[k] = id;
						}
					}
					break;
				}
			}
		}

		void
		fill_submesh(const cgltf_primitive& prim, const ctx& c, submesh_data& out)
		{
			const cgltf_attribute* p = prim.attributes;
			cgltf_size			   n = prim.attributes_count;

			const cgltf_accessor* pos = find_attribute(p, n, cgltf_attribute_type_position, 0);
			out.vertex_count		  = pos ? static_cast<int>(pos->count) : 0;
			if (!pos && n > 0 && p[0].data)
			{
				out.vertex_count = static_cast<int>(p[0].data->count);
			}

			out.position = make_attribute(pos, c);
			out.normal	 = make_attribute(find_attribute(p, n, cgltf_attribute_type_normal, 0), c);
			out.tangent	 = make_attribute(find_attribute(p, n, cgltf_attribute_type_tangent, 0), c);
			out.index	 = make_index_attribute(prim.indices, c);

			fill_attribute_set(p, n, cgltf_attribute_type_texcoord, c, out.p_uv, out.uv_count);
			fill_attribute_set(p, n, cgltf_attribute_type_color, c, out.p_color, out.color_count);
			fill_components_set(p, n, cgltf_attribute_type_color, out.color_count, out.p_color_components);

			int joints_count  = set_count(p, n, cgltf_attribute_type_joints);
			int weights_count = set_count(p, n, cgltf_attribute_type_weights);
			out.joints_count  = joints_count > weights_count ? joints_count : weights_count;
			out.p_joints	  = alloc_arr<index_attribute_view>(out.joints_count);
			out.p_weights	  = alloc_arr<attribute_view>(out.joints_count);
			for (int s = 0; s < out.joints_count; ++s)
			{
				out.p_joints[s]	 = make_index_attribute(find_attribute(p, n, cgltf_attribute_type_joints, s), c);
				out.p_weights[s] = make_attribute(find_attribute(p, n, cgltf_attribute_type_weights, s), c);
			}

			fill_custom_set(p, n, c, out.p_custom, out.custom_count, nullptr, 0);

			out.morph_target_count = static_cast<int>(prim.targets_count);
			out.p_morph_target	   = alloc_arr<morph_target_data>(prim.targets_count);
			for (cgltf_size t = 0; t < prim.targets_count; ++t)
			{
				fill_morph_target(prim.targets[t], c, out.p_morph_target[t]);
			}

			out.has_position_bound = pos && pos->has_min && pos->has_max;
			if (out.has_position_bound)
			{
				std::memcpy(out.position_min, pos->min, sizeof(out.position_min));
				std::memcpy(out.position_max, pos->max, sizeof(out.position_max));
			}

			out.material_idx = idx_of(prim.material, c.p_src->materials);
			out.mode		 = cvt_primitive_mode(prim.type);

			fill_draco(prim, c, out);

			out.extras = cvt_extras(prim.extras);
			fill_extensions(prim.extensions, prim.extensions_count, out.p_extension, out.extension_count);
		}

		void
		free_submesh(submesh_data& s)
		{
			free_attribute(s.position);
			free_attribute(s.normal);
			free_attribute(s.tangent);
			free_index_attribute(s.index);
			for (int i = 0; i < s.uv_count; ++i)
				free_attribute(s.p_uv[i]);
			free_arr(s.p_uv);
			for (int i = 0; i < s.color_count; ++i)
				free_attribute(s.p_color[i]);
			free_arr(s.p_color);
			free_arr(s.p_color_components);
			for (int i = 0; i < s.joints_count; ++i)
			{
				free_index_attribute(s.p_joints[i]);
				free_attribute(s.p_weights[i]);
			}
			free_arr(s.p_joints);
			free_arr(s.p_weights);
			free_custom_set(s.p_custom, s.custom_count);
			for (int i = 0; i < s.morph_target_count; ++i)
				free_morph_target(s.p_morph_target[i]);
			free_arr(s.p_morph_target);
			free_arr(s.p_draco_uv_id);
			free_arr(s.p_draco_color_id);
			free_arr(s.p_draco_joints_id);
			free_arr(s.p_draco_weights_id);
			free_arr(s.p_draco_custom_id);
			free_arr(s.p_extension);
		}

		void
		fill_material_sets(const cgltf_mesh& mesh, const ctx& c, mesh_data& out)
		{
			// unique variant indices used by any primitive of this mesh, ascending
			cgltf_size variant_count = c.p_src->variants_count;
			bool*	   used			 = alloc_arr<bool>(variant_count);
			cgltf_size used_count	 = 0;
			for (cgltf_size p = 0; p < mesh.primitives_count; ++p)
			{
				const cgltf_primitive& prim = mesh.primitives[p];
				for (cgltf_size m = 0; m < prim.mappings_count; ++m)
				{
					cgltf_size v = prim.mappings[m].variant;
					if (v < variant_count && !used[v])
					{
						used[v] = true;
						++used_count;
					}
				}
			}

			out.material_set_count = static_cast<int>(used_count);
			out.p_material_set	   = alloc_arr<material_set_data>(used_count);

			cgltf_size k = 0;
			for (cgltf_size v = 0; v < variant_count; ++v)
			{
				if (!used[v]) continue;
				material_set_data& set = out.p_material_set[k++];
				set.name_idx		   = static_cast<int>(v);
				set.material_count	   = static_cast<int>(mesh.primitives_count);
				set.p_material		   = alloc_arr<int>(mesh.primitives_count);
				set.p_extras		   = alloc_arr<name_view>(mesh.primitives_count);
				for (cgltf_size p = 0; p < mesh.primitives_count; ++p)
				{
					set.p_material[p]			= -1;
					const cgltf_primitive& prim = mesh.primitives[p];
					for (cgltf_size m = 0; m < prim.mappings_count; ++m)
					{
						if (prim.mappings[m].variant == v)
						{
							set.p_material[p] = idx_of(prim.mappings[m].material, c.p_src->materials);
							set.p_extras[p]	  = cvt_extras(prim.mappings[m].extras);
						}
					}
				}
			}
			delete[] used;
		}

		void
		fill_mesh(const cgltf_mesh& mesh, const ctx& c, mesh_data& out)
		{
			out.name = make_name(mesh.name);

			out.submesh_count = static_cast<int>(mesh.primitives_count);
			out.p_submesh	  = alloc_arr<submesh_data>(mesh.primitives_count);
			for (cgltf_size p = 0; p < mesh.primitives_count; ++p)
			{
				fill_submesh(mesh.primitives[p], c, out.p_submesh[p]);
			}

			if (mesh.weights_count)
			{
				float* w = new float[mesh.weights_count];
				std::memcpy(w, mesh.weights, sizeof(float) * mesh.weights_count);
				out.morph_weight = { w, static_cast<int>(mesh.weights_count) };
			}
			fill_names(mesh.target_names, mesh.target_names_count, out.p_morph_target_name, out.morph_target_name_count);

			fill_material_sets(mesh, c, out);

			out.extras = cvt_extras(mesh.extras);
			fill_extensions(mesh.extensions, mesh.extensions_count, out.p_extension, out.extension_count);
		}

		void
		free_mesh(mesh_data& m)
		{
			for (int i = 0; i < m.submesh_count; ++i)
				free_submesh(m.p_submesh[i]);
			free_arr(m.p_submesh);
			delete[] m.morph_weight.p;
			m.morph_weight = {};
			free_arr(m.p_morph_target_name);
			for (int i = 0; i < m.material_set_count; ++i)
			{
				free_arr(m.p_material_set[i].p_material);
				free_arr(m.p_material_set[i].p_extras);
			}
			free_arr(m.p_material_set);
			free_arr(m.p_extension);
		}

		// ---------------------------------------------------------------
		// image / texture / material
		// ---------------------------------------------------------------

		cgltf_size
		calc_base64_size(const char* s)
		{
			cgltf_size n = std::strlen(s);
			while (n > 0 && s[n - 1] == '=')
				--n;
			return (n * 3) / 4;
		}

		void
		fill_image(cgltf_image& img, image_data& out)
		{
			out.name	  = make_name(img.name);
			out.mime_type = make_name(img.mime_type);

			if (img.buffer_view)
			{
				const uint8_t* bytes = cgltf_buffer_view_data(img.buffer_view);
				out.p_embedded		 = bytes;
				out.embedded_size	 = bytes ? static_cast<int>(img.buffer_view->size) : 0;
				return;
			}
			if (!img.uri)
			{
				return;
			}
			if (std::strncmp(img.uri, "data:", 5) == 0)
			{
				const char* comma = std::strchr(img.uri, ',');
				if (comma)
				{
					cgltf_options opt{};
					void*		  data = nullptr;
					cgltf_size	  size = calc_base64_size(comma + 1);
					if (size && cgltf_load_buffer_base64(&opt, size, comma + 1, &data) == cgltf_result_success)
					{
						out.p_embedded		  = static_cast<const unsigned char*>(data);
						out.embedded_size	  = static_cast<int>(size);
						out.is_embedded_owned = true;
					}
					if (!img.mime_type)
					{
						// "data:<mime>;base64,..." -> borrow <mime> by cutting the uri in place
						char* semi = std::strchr(img.uri, ';');
						if (semi && semi < comma)
						{
							*semi		  = '\0';
							out.mime_type = make_name(img.uri + 5);
						}
					}
				}
				return;
			}
			cgltf_decode_uri(img.uri);
			out.uri = make_name(img.uri);
		}

		void
		fill_texture(const cgltf_texture& tex, const ctx& c, texture_data& out)
		{
			out.name			 = make_name(tex.name);
			out.image_idx		 = idx_of(tex.image, c.p_src->images);
			out.basisu_image_idx = idx_of(tex.basisu_image, c.p_src->images);
			out.webp_image_idx	 = idx_of(tex.webp_image, c.p_src->images);

			if (tex.sampler)
			{
				out.sampler_name = make_name(tex.sampler->name);
				out.wrap_s		 = cvt_wrap(tex.sampler->wrap_s);
				out.wrap_t		 = cvt_wrap(tex.sampler->wrap_t);
				out.mag_filter	 = cvt_mag_filter(tex.sampler->mag_filter);
				cvt_min_filter(tex.sampler->min_filter, out.min_filter, out.mipmap);
			}
			else
			{
				out.wrap_s	   = wrap_kind::repeat;
				out.wrap_t	   = wrap_kind::repeat;
				out.mag_filter = filter_kind::undefined;
				out.min_filter = filter_kind::undefined;
				out.mipmap	   = mipmap_kind::none;
			}

			out.extras = cvt_extras(tex.extras);
			fill_extensions(tex.extensions, tex.extensions_count, out.p_extension, out.extension_count);
		}

		texture_slot
		cvt_slot(const cgltf_texture_view& tv, const ctx& c)
		{
			texture_slot s{};
			s.texture_idx		  = idx_of(tv.texture, c.p_src->textures);
			s.texcoord			  = tv.texcoord;
			s.scale				  = tv.scale;
			s.transform.has		  = tv.has_transform != 0;
			s.transform.offset[0] = tv.transform.offset[0];
			s.transform.offset[1] = tv.transform.offset[1];
			s.transform.rotation  = tv.transform.rotation;
			s.transform.scale[0]  = tv.transform.scale[0];
			s.transform.scale[1]  = tv.transform.scale[1];
			s.transform.texcoord  = tv.transform.has_texcoord ? tv.transform.texcoord : -1;
			return s;
		}

		void
		fill_material(const cgltf_material& m, const ctx& c, material_data& out)
		{
			out.name = make_name(m.name);

			out.has_pbr_metallic_roughness = m.has_pbr_metallic_roughness != 0;
			std::memcpy(out.base_color_factor, m.pbr_metallic_roughness.base_color_factor, sizeof(out.base_color_factor));
			out.metallic_factor	 = m.pbr_metallic_roughness.metallic_factor;
			out.roughness_factor = m.pbr_metallic_roughness.roughness_factor;
			std::memcpy(out.emissive_factor, m.emissive_factor, sizeof(out.emissive_factor));
			out.emissive_strength = m.has_emissive_strength ? m.emissive_strength.emissive_strength : 1.f;

			out.alpha_mode	 = cvt_alpha(m.alpha_mode);
			out.alpha_cutoff = m.alpha_cutoff;
			out.double_sided = m.double_sided != 0;

			out.base_color_texture		   = cvt_slot(m.pbr_metallic_roughness.base_color_texture, c);
			out.metallic_roughness_texture = cvt_slot(m.pbr_metallic_roughness.metallic_roughness_texture, c);
			out.normal_texture			   = cvt_slot(m.normal_texture, c);
			out.occlusion_texture		   = cvt_slot(m.occlusion_texture, c);
			out.emissive_texture		   = cvt_slot(m.emissive_texture, c);

			out.feature_mask = cvt_feature_mask(m);
			out.ior			 = m.has_ior ? m.ior.ior : 1.5f;
			out.dispersion	 = m.has_dispersion ? m.dispersion.dispersion : 0.f;

			{
				const cgltf_pbr_specular_glossiness& s				= m.pbr_specular_glossiness;
				out.specular_glossiness.diffuse_texture				= cvt_slot(s.diffuse_texture, c);
				out.specular_glossiness.specular_glossiness_texture = cvt_slot(s.specular_glossiness_texture, c);
				std::memcpy(out.specular_glossiness.diffuse_factor, s.diffuse_factor, sizeof(s.diffuse_factor));
				std::memcpy(out.specular_glossiness.specular_factor, s.specular_factor, sizeof(s.specular_factor));
				out.specular_glossiness.glossiness_factor = s.glossiness_factor;
			}
			{
				const cgltf_clearcoat& s		= m.clearcoat;
				out.clearcoat.clearcoat_texture = cvt_slot(s.clearcoat_texture, c);
				out.clearcoat.roughness_texture = cvt_slot(s.clearcoat_roughness_texture, c);
				out.clearcoat.normal_texture	= cvt_slot(s.clearcoat_normal_texture, c);
				out.clearcoat.factor			= s.clearcoat_factor;
				out.clearcoat.roughness_factor	= s.clearcoat_roughness_factor;
			}
			{
				const cgltf_transmission& s = m.transmission;
				out.transmission.texture	= cvt_slot(s.transmission_texture, c);
				out.transmission.factor		= s.transmission_factor;
			}
			{
				const cgltf_volume& s		 = m.volume;
				out.volume.thickness_texture = cvt_slot(s.thickness_texture, c);
				out.volume.thickness_factor	 = s.thickness_factor;
				std::memcpy(out.volume.attenuation_color, s.attenuation_color, sizeof(s.attenuation_color));
				out.volume.attenuation_distance = s.attenuation_distance;
			}
			{
				const cgltf_sheen& s	= m.sheen;
				out.sheen.color_texture = cvt_slot(s.sheen_color_texture, c);
				std::memcpy(out.sheen.color_factor, s.sheen_color_factor, sizeof(s.sheen_color_factor));
				out.sheen.roughness_texture = cvt_slot(s.sheen_roughness_texture, c);
				out.sheen.roughness_factor	= s.sheen_roughness_factor;
			}
			{
				const cgltf_specular& s		  = m.specular;
				out.specular.specular_texture = cvt_slot(s.specular_texture, c);
				out.specular.color_texture	  = cvt_slot(s.specular_color_texture, c);
				std::memcpy(out.specular.color_factor, s.specular_color_factor, sizeof(s.specular_color_factor));
				out.specular.factor = s.specular_factor;
			}
			{
				const cgltf_iridescence& s		  = m.iridescence;
				out.iridescence.factor			  = s.iridescence_factor;
				out.iridescence.texture			  = cvt_slot(s.iridescence_texture, c);
				out.iridescence.ior				  = s.iridescence_ior;
				out.iridescence.thickness_min	  = s.iridescence_thickness_min;
				out.iridescence.thickness_max	  = s.iridescence_thickness_max;
				out.iridescence.thickness_texture = cvt_slot(s.iridescence_thickness_texture, c);
			}
			{
				const cgltf_diffuse_transmission& s	   = m.diffuse_transmission;
				out.diffuse_transmission.texture	   = cvt_slot(s.diffuse_transmission_texture, c);
				out.diffuse_transmission.factor		   = s.diffuse_transmission_factor;
				out.diffuse_transmission.color_texture = cvt_slot(s.diffuse_transmission_color_texture, c);
				std::memcpy(out.diffuse_transmission.color_factor, s.diffuse_transmission_color_factor, sizeof(s.diffuse_transmission_color_factor));
			}
			{
				const cgltf_anisotropy& s = m.anisotropy;
				out.anisotropy.strength	  = s.anisotropy_strength;
				out.anisotropy.rotation	  = s.anisotropy_rotation;
				out.anisotropy.texture	  = cvt_slot(s.anisotropy_texture, c);
			}

			out.extras = cvt_extras(m.extras);
			fill_extensions(m.extensions, m.extensions_count, out.p_extension, out.extension_count);
		}

		// ---------------------------------------------------------------
		// skin / animation
		// ---------------------------------------------------------------

		void
		fill_skin(const cgltf_skin& skin, const ctx& c, skin_data& out)
		{
			out.name			 = make_name(skin.name);
			out.joint_count		 = static_cast<int>(skin.joints_count);
			out.p_joint_node_idx = alloc_arr<int>(skin.joints_count);
			for (cgltf_size i = 0; i < skin.joints_count; ++i)
			{
				out.p_joint_node_idx[i] = idx_of(skin.joints[i], c.p_src->nodes);
			}
			out.inverse_bind_matrix	   = make_matrix_attribute(skin.inverse_bind_matrices, c);
			out.skeleton_root_node_idx = idx_of(skin.skeleton, c.p_src->nodes);

			out.extras = cvt_extras(skin.extras);
			fill_extensions(skin.extensions, skin.extensions_count, out.p_extension, out.extension_count);
		}

		void
		fill_animation(const cgltf_animation& anim, const ctx& c, animation_data& out)
		{
			out.name = make_name(anim.name);

			out.sampler_count = static_cast<int>(anim.samplers_count);
			out.p_sampler	  = alloc_arr<animation_sampler_data>(anim.samplers_count);
			for (cgltf_size i = 0; i < anim.samplers_count; ++i)
			{
				const cgltf_animation_sampler& s = anim.samplers[i];
				animation_sampler_data&		   d = out.p_sampler[i];
				d.input							 = make_attribute(s.input, c);
				d.output						 = make_attribute(s.output, c);
				d.interpolation					 = cvt_interpolation(s.interpolation);
				d.extras						 = cvt_extras(s.extras);
				fill_extensions(s.extensions, s.extensions_count, d.p_extension, d.extension_count);
			}

			out.channel_count = static_cast<int>(anim.channels_count);
			out.p_channel	  = alloc_arr<animation_channel_data>(anim.channels_count);
			for (cgltf_size i = 0; i < anim.channels_count; ++i)
			{
				const cgltf_animation_channel& ch = anim.channels[i];
				animation_channel_data&		   d  = out.p_channel[i];
				d.sampler_idx					  = idx_of(ch.sampler, anim.samplers);
				d.target_node_idx				  = idx_of(ch.target_node, c.p_src->nodes);
				d.path							  = cvt_animation_path(ch.target_path);
				d.extras						  = cvt_extras(ch.extras);
				fill_extensions(ch.extensions, ch.extensions_count, d.p_extension, d.extension_count);
			}

			out.extras = cvt_extras(anim.extras);
			fill_extensions(anim.extensions, anim.extensions_count, out.p_extension, out.extension_count);
		}

		// ---------------------------------------------------------------
		// node / scene / light / camera
		// ---------------------------------------------------------------

		void
		fill_node(const cgltf_node& node, const ctx& c, node_data& out)
		{
			out.name = make_name(node.name);

			if (node.has_matrix)
			{
				out.local_decompose_trs_failed = decompose_trs(node.matrix, out.local);
			}
			else
			{
				std::memcpy(out.local.translation, node.translation, sizeof(out.local.translation));
				std::memcpy(out.local.rotation, node.rotation, sizeof(out.local.rotation));
				std::memcpy(out.local.scale, node.scale, sizeof(out.local.scale));
				out.local_decompose_trs_failed = false;
			}

			float world[16];
			cgltf_node_transform_world(&node, world);
			transpose_16(world, out.world_matrix);
			out.world_decompose_trs_failed = decompose_trs(world, out.world);

			out.parent_idx		 = idx_of(node.parent, c.p_src->nodes);
			out.child_count		 = static_cast<int>(node.children_count);
			out.p_child_node_idx = new int[node.children_count];
			for (cgltf_size i = 0; i < node.children_count; ++i)
			{
				out.p_child_node_idx[i] = idx_of(node.children[i], c.p_src->nodes);
			}
			out.mesh_idx   = idx_of(node.mesh, c.p_src->meshes);
			out.skin_idx   = idx_of(node.skin, c.p_src->skins);
			out.camera_idx = idx_of(node.camera, c.p_src->cameras);
			out.light_idx  = idx_of(node.light, c.p_src->lights);

			if (node.weights_count)
			{
				float* w = new float[node.weights_count];
				std::memcpy(w, node.weights, sizeof(float) * node.weights_count);
				out.morph_weight = { w, static_cast<int>(node.weights_count) };
			}

			if (node.has_mesh_gpu_instancing)
			{
				const cgltf_attribute*	 p		 = node.mesh_gpu_instancing.attributes;
				cgltf_size				 n		 = node.mesh_gpu_instancing.attributes_count;
				static const char* const known[] = { "TRANSLATION", "ROTATION", "SCALE" };

				const cgltf_attribute* t   = find_attribute_by_name(p, n, known[0]);
				const cgltf_attribute* r   = find_attribute_by_name(p, n, known[1]);
				const cgltf_attribute* s   = find_attribute_by_name(p, n, known[2]);
				const cgltf_attribute* any = t ? t : (r ? r : (s ? s : (n ? &p[0] : nullptr)));

				out.instance_count		 = (any && any->data) ? static_cast<int>(any->data->count) : 0;
				out.instance_translation = make_attribute(t ? t->data : nullptr, c);
				out.instance_rotation	 = make_attribute(r ? r->data : nullptr, c);
				out.instance_scale		 = make_attribute(s ? s->data : nullptr, c);
				fill_custom_set(p, n, c, out.p_instance_custom, out.instance_custom_count, known, 3);
			}

			out.extras = cvt_extras(node.extras);
			fill_extensions(node.extensions, node.extensions_count, out.p_extension, out.extension_count);
		}

		void
		free_node(node_data& n)
		{
			delete[] n.morph_weight.p;
			n.morph_weight = {};
			free_arr(n.p_child_node_idx);
			free_attribute(n.instance_translation);
			free_attribute(n.instance_rotation);
			free_attribute(n.instance_scale);
			free_custom_set(n.p_instance_custom, n.instance_custom_count);
			free_arr(n.p_extension);
		}

		void
		fill_scene(const cgltf_scene& scene, const ctx& c, scene_data& out)
		{
			out.name			= make_name(scene.name);
			out.root_node_count = static_cast<int>(scene.nodes_count);
			out.p_root_node_idx = alloc_arr<int>(scene.nodes_count);
			for (cgltf_size i = 0; i < scene.nodes_count; ++i)
			{
				out.p_root_node_idx[i] = idx_of(scene.nodes[i], c.p_src->nodes);
			}
			out.extras = cvt_extras(scene.extras);
			fill_extensions(scene.extensions, scene.extensions_count, out.p_extension, out.extension_count);
		}

		void
		fill_light(const cgltf_light& l, light_data& out)
		{
			out.name = make_name(l.name);
			out.kind = cvt_light_kind(l.type);
			std::memcpy(out.color, l.color, sizeof(out.color));
			out.intensity			  = l.intensity;
			out.range				  = l.range;
			out.spot_inner_cone_angle = l.spot_inner_cone_angle;
			out.spot_outer_cone_angle = l.spot_outer_cone_angle;
			out.extras				  = cvt_extras(l.extras);
		}

		void
		fill_camera(const cgltf_camera& cam, camera_data& out)
		{
			out.name = make_name(cam.name);
			out.kind = cvt_camera_kind(cam.type);
			if (cam.type == cgltf_camera_type_orthographic)
			{
				const cgltf_camera_orthographic& o = cam.data.orthographic;
				out.xmag						   = o.xmag;
				out.ymag						   = o.ymag;
				out.zfar						   = o.zfar;
				out.znear						   = o.znear;
				out.has_zfar					   = true;
				out.projection_extras			   = cvt_extras(o.extras);
			}
			else
			{
				const cgltf_camera_perspective& p = cam.data.perspective;
				out.has_aspect_ratio			  = p.has_aspect_ratio != 0;
				out.aspect_ratio				  = p.aspect_ratio;
				out.yfov						  = p.yfov;
				out.has_zfar					  = p.has_zfar != 0;
				out.zfar						  = p.zfar;
				out.znear						  = p.znear;
				out.projection_extras			  = cvt_extras(p.extras);
			}
			out.extras = cvt_extras(cam.extras);
			fill_extensions(cam.extensions, cam.extensions_count, out.p_extension, out.extension_count);
		}

		// ---------------------------------------------------------------
		// meshopt block table
		// ---------------------------------------------------------------

		void
		fill_meshopt_blocks(const cgltf_data* src, gltf_data& out, int*& view_to_block)
		{
			view_to_block  = alloc_arr<int>(src->buffer_views_count);
			cgltf_size cnt = 0;
			for (cgltf_size i = 0; i < src->buffer_views_count; ++i)
			{
				view_to_block[i] = -1;
				if (src->buffer_views[i].has_meshopt_compression) ++cnt;
			}
			out.meshopt_block_count = static_cast<int>(cnt);
			out.p_meshopt_block		= alloc_arr<meshopt_block_data>(cnt);
			cgltf_size k			= 0;
			for (cgltf_size i = 0; i < src->buffer_views_count; ++i)
			{
				const cgltf_buffer_view& bv = src->buffer_views[i];
				if (!bv.has_meshopt_compression) continue;
				const cgltf_meshopt_compression& mc = bv.meshopt_compression;
				meshopt_block_data&				 b	= out.p_meshopt_block[k];
				b.p									= (mc.buffer && mc.buffer->data) ? static_cast<const unsigned char*>(mc.buffer->data) + mc.offset : nullptr;
				b.size								= static_cast<int>(mc.size);
				b.stride							= static_cast<int>(mc.stride);
				b.count								= static_cast<int>(mc.count);
				b.mode								= cvt_meshopt_mode(mc.mode);
				b.filter							= cvt_meshopt_filter(mc.filter);
				view_to_block[i]					= static_cast<int>(k);
				++k;
			}
		}

		// ---------------------------------------------------------------
		// print helpers
		// ---------------------------------------------------------------

		const char*
		str(name_view n)
		{
			return n.p ? n.p : "";
		}

		void
		print_av(const char* label, const attribute_view& v)
		{
			if (v.view.p)
			{
				std::printf("%s[%d]", label, v.view.count);
			}
			else if (v.meshopt.block_idx >= 0)
			{
				std::printf("%s{meshopt %d +%d x%d}", label, v.meshopt.block_idx, v.meshopt.byte_offset, v.meshopt.count);
			}
			else
			{
				return;
			}
			std::printf(" ");
		}

		void
		print_iav(const char* label, const index_attribute_view& v)
		{
			if (v.view.p)
			{
				std::printf("%s[%d]", label, v.view.count);
			}
			else if (v.meshopt.block_idx >= 0)
			{
				std::printf("%s{meshopt %d +%d x%d}", label, v.meshopt.block_idx, v.meshopt.byte_offset, v.meshopt.count);
			}
			else
			{
				return;
			}
			std::printf(" ");
		}

		void
		print_slot(const char* label, const texture_slot& s)
		{
			if (s.texture_idx < 0) return;
			std::printf("      %s: tex %d uv %d scale %g", label, s.texture_idx, s.texcoord, s.scale);
			if (s.transform.has)
			{
				std::printf(" xform(off %g %g rot %g scale %g %g uv %d)",
							s.transform.offset[0], s.transform.offset[1], s.transform.rotation,
							s.transform.scale[0], s.transform.scale[1], s.transform.texcoord);
			}
			std::printf("\n");
		}
	}	 // namespace

	// -------------------------------------------------------------------
	// public
	// -------------------------------------------------------------------

	detail::gltf_data
	load_gltf(const char* full_path)
	{
		gltf_data out{};
		out.default_scene_idx = -1;

		cgltf_options options{};
		cgltf_data*	  src = nullptr;

		cgltf_result r = cgltf_parse_file(&options, full_path, &src);
		if (r == cgltf_result_success) r = cgltf_load_buffers(&options, src, full_path);
		if (r == cgltf_result_success) r = cgltf_validate(src);
		if (r != cgltf_result_success)
		{
			cgltf_free(src);
			out.error = cvt_error(r);
			return out;
		}

		out.p_internal = src;
		out.error	   = load_error::none;

		ctx c{};
		c.p_src = src;
		fill_meshopt_blocks(src, out, c.p_view_to_block);

		out.generator	 = make_name(src->asset.generator);
		out.copyright	 = make_name(src->asset.copyright);
		out.version		 = make_name(src->asset.version);
		out.min_version	 = make_name(src->asset.min_version);
		out.asset_extras = cvt_extras(src->asset.extras);
		fill_extensions(src->asset.extensions, src->asset.extensions_count, out.p_asset_extension, out.asset_extension_count);

		out.mesh_count = static_cast<int>(src->meshes_count);
		out.p_mesh	   = alloc_arr<mesh_data>(src->meshes_count);
		for (cgltf_size i = 0; i < src->meshes_count; ++i)
			fill_mesh(src->meshes[i], c, out.p_mesh[i]);

		out.image_count = static_cast<int>(src->images_count);
		out.p_image		= alloc_arr<image_data>(src->images_count);
		for (cgltf_size i = 0; i < src->images_count; ++i)
			fill_image(src->images[i], out.p_image[i]);

		out.texture_count = static_cast<int>(src->textures_count);
		out.p_texture	  = alloc_arr<texture_data>(src->textures_count);
		for (cgltf_size i = 0; i < src->textures_count; ++i)
			fill_texture(src->textures[i], c, out.p_texture[i]);

		out.material_count = static_cast<int>(src->materials_count);
		out.p_material	   = alloc_arr<material_data>(src->materials_count);
		for (cgltf_size i = 0; i < src->materials_count; ++i)
			fill_material(src->materials[i], c, out.p_material[i]);

		out.skin_count = static_cast<int>(src->skins_count);
		out.p_skin	   = alloc_arr<skin_data>(src->skins_count);
		for (cgltf_size i = 0; i < src->skins_count; ++i)
			fill_skin(src->skins[i], c, out.p_skin[i]);

		out.animation_count = static_cast<int>(src->animations_count);
		out.p_animation		= alloc_arr<animation_data>(src->animations_count);
		for (cgltf_size i = 0; i < src->animations_count; ++i)
			fill_animation(src->animations[i], c, out.p_animation[i]);

		out.node_count = static_cast<int>(src->nodes_count);
		out.p_node	   = alloc_arr<node_data>(src->nodes_count);
		for (cgltf_size i = 0; i < src->nodes_count; ++i)
			fill_node(src->nodes[i], c, out.p_node[i]);

		out.scene_count = static_cast<int>(src->scenes_count);
		out.p_scene		= alloc_arr<scene_data>(src->scenes_count);
		for (cgltf_size i = 0; i < src->scenes_count; ++i)
			fill_scene(src->scenes[i], c, out.p_scene[i]);

		out.light_count = static_cast<int>(src->lights_count);
		out.p_light		= alloc_arr<light_data>(src->lights_count);
		for (cgltf_size i = 0; i < src->lights_count; ++i)
			fill_light(src->lights[i], out.p_light[i]);

		out.camera_count = static_cast<int>(src->cameras_count);
		out.p_camera	 = alloc_arr<camera_data>(src->cameras_count);
		for (cgltf_size i = 0; i < src->cameras_count; ++i)
			fill_camera(src->cameras[i], out.p_camera[i]);

		out.material_set_name_count = static_cast<int>(src->variants_count);
		out.p_material_set_name		= alloc_arr<name_view>(src->variants_count);
		out.p_material_set_extras	= alloc_arr<name_view>(src->variants_count);
		for (cgltf_size i = 0; i < src->variants_count; ++i)
		{
			out.p_material_set_name[i]	 = make_name(src->variants[i].name);
			out.p_material_set_extras[i] = cvt_extras(src->variants[i].extras);
		}

		fill_names(src->extensions_required, src->extensions_required_count, out.p_required_extension, out.required_extension_count);
		fill_names(src->extensions_used, src->extensions_used_count, out.p_used_extension, out.used_extension_count);

		out.extras = cvt_extras(src->extras);
		fill_extensions(src->data_extensions, src->data_extensions_count, out.p_extension, out.extension_count);

		out.default_scene_idx = idx_of(src->scene, src->scenes);

		delete[] c.p_view_to_block;
		return out;
	}

	void
	release(detail::gltf_data& d)
	{
		for (int i = 0; i < d.mesh_count; ++i)
			free_mesh(d.p_mesh[i]);
		free_arr(d.p_mesh);

		for (int i = 0; i < d.image_count; ++i)
		{
			if (d.p_image[i].is_embedded_owned)
			{
				std::free(const_cast<unsigned char*>(d.p_image[i].p_embedded));
			}
			free_arr(d.p_image[i].p_extension);
		}
		free_arr(d.p_image);

		for (int i = 0; i < d.texture_count; ++i)
			free_arr(d.p_texture[i].p_extension);
		free_arr(d.p_texture);

		for (int i = 0; i < d.material_count; ++i)
			free_arr(d.p_material[i].p_extension);
		free_arr(d.p_material);

		for (int i = 0; i < d.skin_count; ++i)
		{
			free_arr(d.p_skin[i].p_joint_node_idx);
			free_attribute(d.p_skin[i].inverse_bind_matrix);
			free_arr(d.p_skin[i].p_extension);
		}
		free_arr(d.p_skin);

		for (int i = 0; i < d.animation_count; ++i)
		{
			animation_data& a = d.p_animation[i];
			for (int s = 0; s < a.sampler_count; ++s)
			{
				free_attribute(a.p_sampler[s].input);
				free_attribute(a.p_sampler[s].output);
				free_arr(a.p_sampler[s].p_extension);
			}
			free_arr(a.p_sampler);
			for (int ch = 0; ch < a.channel_count; ++ch)
				free_arr(a.p_channel[ch].p_extension);
			free_arr(a.p_channel);
			free_arr(a.p_extension);
		}
		free_arr(d.p_animation);

		for (int i = 0; i < d.node_count; ++i)
			free_node(d.p_node[i]);
		free_arr(d.p_node);

		for (int i = 0; i < d.scene_count; ++i)
		{
			free_arr(d.p_scene[i].p_root_node_idx);
			free_arr(d.p_scene[i].p_extension);
		}
		free_arr(d.p_scene);

		free_arr(d.p_light);

		for (int i = 0; i < d.camera_count; ++i)
			free_arr(d.p_camera[i].p_extension);
		free_arr(d.p_camera);

		free_arr(d.p_material_set_name);
		free_arr(d.p_material_set_extras);
		free_arr(d.p_meshopt_block);
		free_arr(d.p_required_extension);
		free_arr(d.p_used_extension);
		free_arr(d.p_extension);
		free_arr(d.p_asset_extension);

		cgltf_free(static_cast<cgltf_data*>(d.p_internal));
		d					= {};
		d.error				= load_error::none;
		d.default_scene_idx = -1;
	}

	void
	print_gltf(const detail::gltf_data& d)
	{
		if (d.error != load_error::none)
		{
			std::printf("gltf: load error %d\n", static_cast<int>(d.error));
			return;
		}

		std::printf("gltf: generator '%s' version '%s'\n", str(d.generator), str(d.version));
		std::printf("  mesh %d image %d texture %d material %d skin %d animation %d node %d scene %d light %d camera %d\n",
					d.mesh_count, d.image_count, d.texture_count, d.material_count, d.skin_count,
					d.animation_count, d.node_count, d.scene_count, d.light_count, d.camera_count);
		std::printf("  variants %d meshopt blocks %d default scene %d\n", d.material_set_name_count, d.meshopt_block_count, d.default_scene_idx);
		for (int i = 0; i < d.required_extension_count; ++i)
			std::printf("  required: %s\n", str(d.p_required_extension[i]));
		for (int i = 0; i < d.used_extension_count; ++i)
			std::printf("  used: %s\n", str(d.p_used_extension[i]));
		if (d.extras.p) std::printf("  extras: %s\n", d.extras.p);

		for (int i = 0; i < d.mesh_count; ++i)
		{
			const mesh_data& m = d.p_mesh[i];
			std::printf("mesh %d '%s': submesh %d morph weights %d sets %d\n", i, str(m.name), m.submesh_count, m.morph_weight.count, m.material_set_count);
			for (int t = 0; t < m.morph_target_name_count; ++t)
				std::printf("    target name %d: %s\n", t, str(m.p_morph_target_name[t]));
			for (int s = 0; s < m.submesh_count; ++s)
			{
				const submesh_data& sm = m.p_submesh[s];
				std::printf("  submesh %d: verts %d mode %d material %d uv %d color %d joints %d custom %d morph %d%s\n",
							s, sm.vertex_count, static_cast<int>(sm.mode), sm.material_idx, sm.uv_count, sm.color_count,
							sm.joints_count, sm.custom_count, sm.morph_target_count, sm.p_draco ? " draco" : "");
				std::printf("    ");
				print_av("pos", sm.position);
				print_av("nrm", sm.normal);
				print_av("tan", sm.tangent);
				print_iav("idx", sm.index);
				for (int k = 0; k < sm.uv_count; ++k)
					print_av("uv", sm.p_uv[k]);
				for (int k = 0; k < sm.color_count; ++k)
					print_av("col", sm.p_color[k]);
				for (int k = 0; k < sm.joints_count; ++k)
				{
					print_iav("jnt", sm.p_joints[k]);
					print_av("wgt", sm.p_weights[k]);
				}
				for (int k = 0; k < sm.custom_count; ++k)
					print_av(str(sm.p_custom[k].name), sm.p_custom[k].data);
				std::printf("\n");
				if (sm.has_position_bound)
				{
					std::printf("    bound min %g %g %g max %g %g %g\n", sm.position_min[0], sm.position_min[1], sm.position_min[2],
								sm.position_max[0], sm.position_max[1], sm.position_max[2]);
				}
				for (int t = 0; t < sm.morph_target_count; ++t)
				{
					const morph_target_data& mt = sm.p_morph_target[t];
					std::printf("    morph %d: ", t);
					print_av("dpos", mt.position_delta);
					print_av("dnrm", mt.normal_delta);
					print_av("dtan", mt.tangent_delta);
					for (int k = 0; k < mt.uv_delta_count; ++k)
						print_av("duv", mt.p_uv_delta[k]);
					for (int k = 0; k < mt.color_delta_count; ++k)
						print_av("dcol", mt.p_color_delta[k]);
					std::printf("\n");
				}
			}
			for (int s = 0; s < m.material_set_count; ++s)
			{
				const material_set_data& set = m.p_material_set[s];
				std::printf("  set '%s':", str(d.p_material_set_name[set.name_idx]));
				for (int k = 0; k < set.material_count; ++k)
					std::printf(" %d", set.p_material[k]);
				std::printf("\n");
			}
		}

		for (int i = 0; i < d.image_count; ++i)
		{
			const image_data& im = d.p_image[i];
			std::printf("image %d '%s': uri '%s' mime '%s' embedded %d%s\n", i, str(im.name), str(im.uri), str(im.mime_type),
						im.embedded_size, im.is_embedded_owned ? " (owned)" : "");
		}
		for (int i = 0; i < d.texture_count; ++i)
		{
			const texture_data& t = d.p_texture[i];
			std::printf("texture %d '%s': image %d basisu %d webp %d wrap %d %d mag %d min %d mip %d\n", i, str(t.name), t.image_idx,
						t.basisu_image_idx, t.webp_image_idx, static_cast<int>(t.wrap_s), static_cast<int>(t.wrap_t),
						static_cast<int>(t.mag_filter), static_cast<int>(t.min_filter), static_cast<int>(t.mipmap));
		}
		for (int i = 0; i < d.material_count; ++i)
		{
			const material_data& m = d.p_material[i];
			std::printf("material %d '%s': pbr %d base %g %g %g %g metal %g rough %g emissive %g %g %g x%g alpha %d cutoff %g double %d features 0x%x ior %g\n",
						i, str(m.name), m.has_pbr_metallic_roughness, m.base_color_factor[0], m.base_color_factor[1], m.base_color_factor[2],
						m.base_color_factor[3], m.metallic_factor, m.roughness_factor, m.emissive_factor[0], m.emissive_factor[1],
						m.emissive_factor[2], m.emissive_strength, static_cast<int>(m.alpha_mode), m.alpha_cutoff, m.double_sided,
						m.feature_mask, m.ior);
			print_slot("base_color", m.base_color_texture);
			print_slot("metallic_roughness", m.metallic_roughness_texture);
			print_slot("normal", m.normal_texture);
			print_slot("occlusion", m.occlusion_texture);
			print_slot("emissive", m.emissive_texture);
			if (m.feature_mask & material_feature_specular_glossiness)
			{
				std::printf("      spec_gloss: diffuse %g %g %g %g specular %g %g %g gloss %g\n",
							m.specular_glossiness.diffuse_factor[0], m.specular_glossiness.diffuse_factor[1],
							m.specular_glossiness.diffuse_factor[2], m.specular_glossiness.diffuse_factor[3],
							m.specular_glossiness.specular_factor[0], m.specular_glossiness.specular_factor[1],
							m.specular_glossiness.specular_factor[2], m.specular_glossiness.glossiness_factor);
				print_slot("sg_diffuse", m.specular_glossiness.diffuse_texture);
				print_slot("sg_specular_glossiness", m.specular_glossiness.specular_glossiness_texture);
			}
		}
		for (int i = 0; i < d.skin_count; ++i)
		{
			const skin_data& s = d.p_skin[i];
			std::printf("skin %d '%s': joints %d root %d ", i, str(s.name), s.joint_count, s.skeleton_root_node_idx);
			print_av("ibm", s.inverse_bind_matrix);
			std::printf("\n");
		}
		for (int i = 0; i < d.animation_count; ++i)
		{
			const animation_data& a = d.p_animation[i];
			std::printf("animation %d '%s': samplers %d channels %d\n", i, str(a.name), a.sampler_count, a.channel_count);
			for (int s = 0; s < a.sampler_count; ++s)
			{
				std::printf("  sampler %d: interp %d ", s, static_cast<int>(a.p_sampler[s].interpolation));
				print_av("in", a.p_sampler[s].input);
				print_av("out", a.p_sampler[s].output);
				std::printf("\n");
			}
			for (int ch = 0; ch < a.channel_count; ++ch)
			{
				std::printf("  channel %d: sampler %d node %d path %d\n", ch, a.p_channel[ch].sampler_idx,
							a.p_channel[ch].target_node_idx, static_cast<int>(a.p_channel[ch].path));
			}
		}
		for (int i = 0; i < d.node_count; ++i)
		{
			const node_data& n = d.p_node[i];
			std::printf("node %d '%s': parent %d mesh %d skin %d camera %d light %d%s%s\n", i, str(n.name), n.parent_idx, n.mesh_idx,
						n.skin_idx, n.camera_idx, n.light_idx, n.local_decompose_trs_failed ? " local_decompose_trs_failed" : "", n.world_decompose_trs_failed ? " world_decompose_trs_failed" : "");
			std::printf("  local t %g %g %g r %g %g %g %g s %g %g %g\n", n.local.translation[0], n.local.translation[1],
						n.local.translation[2], n.local.rotation[0], n.local.rotation[1], n.local.rotation[2], n.local.rotation[3],
						n.local.scale[0], n.local.scale[1], n.local.scale[2]);
			std::printf("  world t %g %g %g r %g %g %g %g s %g %g %g\n", n.world.translation[0], n.world.translation[1],
						n.world.translation[2], n.world.rotation[0], n.world.rotation[1], n.world.rotation[2], n.world.rotation[3],
						n.world.scale[0], n.world.scale[1], n.world.scale[2]);
			if (n.morph_weight.count) std::printf("  morph weights %d\n", n.morph_weight.count);
			if (n.instance_count)
			{
				std::printf("  instances %d ", n.instance_count);
				print_av("t", n.instance_translation);
				print_av("r", n.instance_rotation);
				print_av("s", n.instance_scale);
				for (int k = 0; k < n.instance_custom_count; ++k)
					print_av(str(n.p_instance_custom[k].name), n.p_instance_custom[k].data);
				std::printf("\n");
			}
		}
		for (int i = 0; i < d.scene_count; ++i)
		{
			const scene_data& s = d.p_scene[i];
			std::printf("scene %d '%s': roots", i, str(s.name));
			for (int k = 0; k < s.root_node_count; ++k)
				std::printf(" %d", s.p_root_node_idx[k]);
			std::printf("\n");
		}
		for (int i = 0; i < d.light_count; ++i)
		{
			const light_data& l = d.p_light[i];
			std::printf("light %d '%s': kind %d color %g %g %g intensity %g range %g cone %g %g\n", i, str(l.name), static_cast<int>(l.kind),
						l.color[0], l.color[1], l.color[2], l.intensity, l.range, l.spot_inner_cone_angle, l.spot_outer_cone_angle);
		}
		for (int i = 0; i < d.camera_count; ++i)
		{
			const camera_data& c = d.p_camera[i];
			std::printf("camera %d '%s': kind %d aspect %d:%g yfov %g zfar %d:%g znear %g xmag %g ymag %g\n", i, str(c.name),
						static_cast<int>(c.kind), c.has_aspect_ratio, c.aspect_ratio, c.yfov, c.has_zfar, c.zfar, c.znear, c.xmag, c.ymag);
		}
		for (int i = 0; i < d.meshopt_block_count; ++i)
		{
			const meshopt_block_data& b = d.p_meshopt_block[i];
			std::printf("meshopt block %d: size %d stride %d count %d mode %d filter %d\n", i, b.size, b.stride, b.count,
						static_cast<int>(b.mode), static_cast<int>(b.filter));
		}
	}
}	 // namespace age::external::cgltf