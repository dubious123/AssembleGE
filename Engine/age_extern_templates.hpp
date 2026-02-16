#pragma once
#include "age_pch.hpp"

namespace age::inline data_structure
{
#define extern_template(template_name, t_type) extern template struct template_name<t_type>
#define value_types                \
	uint8, uint16, uint32, uint64, \
		int8, int16, int32, int64, \
		float, double, half
#define container_types vector, stable_dense_vector /*, sparse_vector*/

#define vec_types_impl(type) vec2<type>, vec2a<type>, vec3<type>, vec3a<type>, vec4<type>, vec4a<type>
#define mat_types_impl(type) mat22<type>, mat33<type>, mat44<type>, mat22a<type>, mat33a<type>, mat44a<type>

#define inst_template_step_0(tpl)	 inst_template_step_1 tpl
#define inst_template_step_1(x, ...) FOR_EACH_CTX((extern_template, x), AGE_PP_SEMICOLON_I, __VA_ARGS__)

	FOR_EACH_SEP(inst_template_step_0, AGE_PP_SEMICOLON_I,
				 AGE_PP_CARTESIAN_PRODUCT((container_types),
										  (FOR_EACH_ARG(vec_types_impl, value_types),
										   FOR_EACH_ARG(mat_types_impl, value_types))));

#undef inst_template_step_1
#undef inst_template_step_0

#undef mat_types_impl
#undef vec_types_impl
#undef container_types
#undef value_types
#undef extern_template

}	 // namespace age::inline data_structure
