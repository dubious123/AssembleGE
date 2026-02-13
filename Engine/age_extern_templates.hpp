#pragma once
#include "age_pch.hpp"

namespace age::inline data_structure
{
#define instantiate_types          \
	uint8, uint16, uint32, uint64, \
		int8, int16, int32, int64, \
		float, double, half

#define vec_types_impl(type)		   vec2<type>, vec2a<type>, vec3<type>, vec3a<type>, vec4<type>, vec4a<type>
#define mat_types_impl(type)		   mat22<type>, mat33<type>, mat44<type>, mat22a<type>, mat33a<type>, mat44a<type>
#define extern_template_vector(t_type) extern template struct age::data_structure::vector<t_type>

	FOR_EACH(extern_template_vector,
			 FOR_EACH_ARG(vec_types_impl, instantiate_types),
			 FOR_EACH_ARG(mat_types_impl, instantiate_types));

#undef extern_template_vector
#undef mat_types_impl
#undef vec_types_impl
#undef instantiate_types
}	 // namespace age::inline data_structure
