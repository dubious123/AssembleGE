#pragma once
#include "age.hpp"

namespace age::graphics
{
	namespace where
	{
		namespace e
		{
			enum class reg_kind : uint8
			{
				b = 1 << 0,
				t = 1 << 1,
				u = 1 << 2,
				s = 1 << 3
			};
		}

		template <e::reg_kind kind_, uint32 idx, uint32 space = 0>
		struct reg
		{
			static constexpr e::reg_kind kind			= kind_;
			static constexpr uint32_t	 register_index = idx;
			static constexpr uint32_t	 register_space = space;
		};

		template <uint32 n, uint32 space = 0>
		using b = reg<e::reg_kind::b, n, space>;	// Constant Buffer

		template <uint32 n, uint32 space = 0>
		using t = reg<e::reg_kind::t, n, space>;	// SRV (Structured/ByteAddress)

		template <uint32 n, uint32 space = 0>
		using u = reg<e::reg_kind::u, n, space>;	// UAV

		template <uint32 n, uint32 space = 0>
		using s = reg<e::reg_kind::s, n, space>;	// Sampler

													// template <typename t_reg_base, std::size_t n>
	}	 // namespace where

	namespace how
	{
		class root_constant { };

		class root_descriptor { };

		class static_sampler { };
	}	 // namespace how

	namespace what
	{
		template <typename t_data_, std::size_t n = g::frame_buffer_count>
		struct constant_buffer_array
		{
			static_assert(n > 0);

			using t_data = t_data_;

			static constexpr auto array_size = n;
		};

		template <std::size_t n = g::frame_buffer_count>
		struct byte_address_buffer_array
		{
			static_assert(n > 0);
			static constexpr auto array_size = n;

			using t_data = std::byte;
		};

		template <typename t_data_, std::size_t n = g::frame_buffer_count>
		struct structured_buffer_array
		{
			static_assert(n > 0);
			static_assert(sizeof(t_data_) % 4 == 0);

			using t_data = t_data_;

			static constexpr auto array_size = n;
		};

		template <typename t_data>
		class constant_buffer : public constant_buffer_array<t_data, 1> { };

		template <std::size_t n = g::frame_buffer_count>
		class rw_byte_address_buffer_array : public byte_address_buffer_array<n> { };

		class byte_address_buffer : public byte_address_buffer_array<1> { };

		class rw_byte_address_buffer : public byte_address_buffer_array<1> { };

		template <typename t_data, std::size_t n = g::frame_buffer_count>
		class rw_structured_buffer_array : public structured_buffer_array<t_data, n> { };

		template <typename t_data>
		class structured_buffer : public structured_buffer_array<t_data, 1> { };

		template <typename t_data>
		class rw_structured_buffer : public structured_buffer_array<t_data, 1> { };

		template <auto fn>
		struct sampler
		{
			static constexpr auto factory = fn;
		};
	}	 // namespace what

	namespace detail
	{
		template <typename t>
		class is_constant_buffer : public std::false_type { };

		template <typename t>
		class is_constant_buffer<what::constant_buffer<t>> : public std::true_type { };

		template <typename t>
		class is_structured_buffer : public std::false_type { };

		template <typename t>
		class is_structured_buffer<what::structured_buffer<t>> : public std::true_type { };

		template <typename t>
		class is_rw_structured_buffer : public std::false_type { };

		template <typename t>
		class is_rw_structured_buffer<what::rw_structured_buffer<t>> : public std::true_type { };

		template <typename t>
		class is_byte_address_buffer : public std::false_type { };

		template <>
		class is_byte_address_buffer<what::byte_address_buffer> : public std::true_type { };

		template <typename t>
		class is_rw_byte_address_buffer : public std::false_type { };

		template <>
		class is_rw_byte_address_buffer<what::rw_byte_address_buffer> : public std::true_type { };

		template <typename t>
		class is_constant_buffer_array : public std::false_type { };

		template <typename t, std::size_t n>
		class is_constant_buffer_array<what::constant_buffer_array<t, n>> : public std::true_type { };

		template <typename t>
		class is_structured_buffer_array : public std::false_type { };

		template <typename t, std::size_t n>
		class is_structured_buffer_array<what::structured_buffer_array<t, n>> : public std::true_type { };

		template <typename t>
		class is_byte_address_buffer_array : public std::false_type { };

		template <std::size_t n>
		class is_byte_address_buffer_array<what::byte_address_buffer_array<n>> : public std::true_type { };

		template <typename t>
		class is_rw_structured_buffer_array : public std::false_type { };

		template <typename t, std::size_t n>
		class is_rw_structured_buffer_array<what::rw_structured_buffer_array<t, n>> : public std::true_type { };

		template <typename t>
		class is_rw_byte_address_buffer_array : public std::false_type { };

		template <std::size_t n>
		class is_rw_byte_address_buffer_array<what::rw_byte_address_buffer_array<n>> : public std::true_type { };

		template <typename t>
		inline constexpr bool is_constant_buffer_v = is_constant_buffer<std::remove_cvref_t<t>>::value;

		template <typename t>
		inline constexpr bool is_constant_buffer_array_v = is_constant_buffer_array<std::remove_cvref_t<t>>::value;

		template <typename t>
		inline constexpr bool is_structured_buffer_v = is_structured_buffer<std::remove_cvref_t<t>>::value;

		template <typename t>
		inline constexpr bool is_structured_buffer_array_v = is_structured_buffer_array<std::remove_cvref_t<t>>::value;

		template <typename t>
		inline constexpr bool is_byte_address_buffer_v = is_byte_address_buffer<std::remove_cvref_t<t>>::value;

		template <typename t>
		inline constexpr bool is_byte_address_buffer_array_v = is_byte_address_buffer_array<std::remove_cvref_t<t>>::value;

		template <typename t>
		inline constexpr bool is_rw_structured_buffer_v = is_rw_structured_buffer<std::remove_cvref_t<t>>::value;

		template <typename t>
		inline constexpr bool is_rw_byte_address_buffer_v = is_rw_byte_address_buffer<std::remove_cvref_t<t>>::value;

		template <typename t>
		inline constexpr bool is_rw_structured_buffer_array_v = is_rw_structured_buffer_array<std::remove_cvref_t<t>>::value;

		template <typename t>
		inline constexpr bool is_rw_byte_address_buffer_array_v = is_rw_byte_address_buffer_array<std::remove_cvref_t<t>>::value;

		template <typename t_what>
		inline constexpr bool is_root_descriptor_compatible_v = is_constant_buffer_v<t_what>
															 or is_constant_buffer_array_v<t_what>
															 or is_structured_buffer_v<t_what>
															 or is_structured_buffer_array_v<t_what>
															 or is_rw_structured_buffer_v<t_what>
															 or is_rw_structured_buffer_array_v<t_what>
															 or is_byte_address_buffer_v<t_what>
															 or is_byte_address_buffer_array_v<t_what>
															 or is_rw_byte_address_buffer_v<t_what>
															 or is_rw_byte_address_buffer_array_v<t_what>;

		template <typename t>
		class is_sampler : public std::false_type { };

		template <auto i>
		class is_sampler<what::sampler<i>> : public std::true_type { };

		template <typename t>
		inline constexpr bool is_sampler_v = is_sampler<std::remove_cvref_t<t>>::value;
	}	 // namespace detail

	template <util::nttp_string_holder str_nttp, auto, D3D12_SHADER_VISIBILITY, typename t_what, typename t_how, typename t_where, std::size_t slot_id = -1>
	struct binding_slot
	{
		static_assert(false);
	};

	template <util::nttp_string_holder str_nttp, D3D12_SHADER_VISIBILITY visibility, typename t_what, typename t_where_, std::size_t slot_id_>
	requires(detail::is_constant_buffer_v<t_what> || detail::is_constant_buffer_array_v<t_what>)
	struct binding_slot<str_nttp, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, visibility, t_what, how::root_constant, t_where_, slot_id_>
	{
		using t_data  = typename t_what::t_data;
		using t_how	  = how::root_constant;
		using t_where = t_where_;

		static constexpr auto shader_visibility = visibility;
		static constexpr auto slot_id			= slot_id_;

		static_assert(sizeof(t_data) <= 256, "root_constant data is too large");
		static_assert(sizeof(t_data) % 4 == 0, "root_constant data size must be a multiple of 4");

		std::array<t_data, t_what::array_size> constant_arr;

		void
		bind(const t_data& data, uint8 ring_idx = g::frame_buffer_idx) noexcept
			requires(constant_arr.size() > 1)
		{
			constant_arr[ring_idx] = data;
		}

		void
		bind(const t_data& data) noexcept
			requires(constant_arr.size() == 1)
		{
			constant_arr[0] = data;
		}

		void
		apply(t_cmd_list& cmd_list, uint8 ring_idx = g::frame_buffer_idx) noexcept
			requires(constant_arr.size() > 1)
		{
			AGE_ASSERT(slot_id != -1, "Invalid slot_id");
			cmd_list.SetGraphicsRoot32BitConstants(slot_id, sizeof(t_data) / 4, &constant_arr[ring_idx], 0);
		}

		void
		apply(t_cmd_list& cmd_list) noexcept
			requires(constant_arr.size() == 1)
		{
			AGE_ASSERT(slot_id != -1, "Invalid slot_id");
			cmd_list.SetGraphicsRoot32BitConstants(slot_id, sizeof(t_data) / 4, &constant_arr[0], 0);
		}
	};

	template <util::nttp_string_holder str_nttp, auto flags, D3D12_SHADER_VISIBILITY visibility, typename t_what, typename t_where_, std::size_t slot_id_>
	requires detail::is_root_descriptor_compatible_v<t_what>
	struct binding_slot<str_nttp, flags, visibility, t_what, how::root_descriptor, t_where_, slot_id_>
	{
		using t_how	  = how::root_descriptor;
		using t_where = t_where_;

		static constexpr auto descriptor_flags	= flags;
		static constexpr auto shader_visibility = visibility;
		static constexpr auto slot_id			= slot_id_;

		std::array<D3D12_GPU_VIRTUAL_ADDRESS, t_what::array_size> gpu_va_arr;

		static_assert(not(detail::is_constant_buffer_v<t_what> or detail::is_constant_buffer_array_v<t_what>)
					  or (sizeof(typename t_what::t_data) % 256 == 0));

		void
		bind(D3D12_GPU_VIRTUAL_ADDRESS va) noexcept
			requires(gpu_va_arr.size() == 1)
		{
			bind_impl(va, 0);
		}

		void
		bind(D3D12_GPU_VIRTUAL_ADDRESS va, uint8 ring_idx = g::frame_buffer_idx) noexcept
			requires(gpu_va_arr.size() > 1)
		{
			bind_impl(va, ring_idx);
		}

		void
		apply(t_cmd_list& cmd_list) noexcept
			requires(gpu_va_arr.size() == 1)
		{
			AGE_ASSERT(slot_id != -1, "Invalid slot_id");
			if constexpr (detail::is_constant_buffer_v<t_what>)
			{
				cmd_list.SetGraphicsRootConstantBufferView(slot_id, gpu_va_arr[0]);
			}
			else if constexpr (detail::is_structured_buffer_v<t_what> or detail::is_byte_address_buffer_v<t_what>)
			{
				cmd_list.SetGraphicsRootShaderResourceView(slot_id, gpu_va_arr[0]);
			}
			else
			{
				cmd_list.SetGraphicsRootUnorderedAccessView(slot_id, gpu_va_arr[0]);
			}
		}

		void
		apply(t_cmd_list& cmd_list, std::size_t ring_idx = g::frame_buffer_idx) noexcept
			requires(gpu_va_arr.size() > 1)
		{
			AGE_ASSERT(slot_id != -1, "Invalid slot_id");
			if constexpr (detail::is_constant_buffer_array_v<t_what>)
			{
				cmd_list.SetGraphicsRootConstantBufferView(slot_id, gpu_va_arr[ring_idx]);
			}
			else if constexpr (detail::is_structured_buffer_array_v<t_what> or detail::is_byte_address_buffer_array_v<t_what>)
			{
				cmd_list.SetGraphicsRootShaderResourceView(slot_id, gpu_va_arr[ring_idx]);
			}
			else
			{
				cmd_list.SetGraphicsRootUnorderedAccessView(slot_id, gpu_va_arr[ring_idx]);
			}
		}

		D3D12_GPU_VIRTUAL_ADDRESS
		get_va(std::size_t ring_idx = g::frame_buffer_idx) noexcept
		{
			return gpu_va_arr[ring_idx];
		}

	  private:
		void
		bind_impl(D3D12_GPU_VIRTUAL_ADDRESS va, auto ring_idx) noexcept
		{
			AGE_ASSERT(va != 0, "GPU Virtual Address cannot be null");

			if constexpr (detail::is_constant_buffer_v<t_what> or detail::is_constant_buffer_array_v<t_what>)
			{
				AGE_ASSERT(va % 256 == 0, "CBV Address must be 256-byte aligned");
			}
			else
			{
				AGE_ASSERT(va % 4 == 0, "SRV/UAV Address must be 4-byte aligned");
			}

			gpu_va_arr[ring_idx] = va;
		}
	};

	template <util::nttp_string_holder str_nttp, D3D12_SAMPLER_FLAGS flags, D3D12_SHADER_VISIBILITY visibility, typename t_what_, typename t_where_, std::size_t slot_id_>
	requires(detail::is_sampler_v<t_what_>)
	struct binding_slot<str_nttp, flags, visibility, t_what_, how::static_sampler, t_where_, slot_id_>
	{
		using t_how	  = how::static_sampler;
		using t_where = t_where_;
		using t_what  = t_what_;

		static constexpr auto sampler_flags		= flags;
		static constexpr auto shader_visibility = visibility;
		static constexpr auto slot_id			= slot_id_;
	};

	namespace detail
	{
		template <typename t>
		struct is_binding_slot : std::false_type
		{
		};

		template <util::nttp_string_holder str_nttp, auto n0, D3D12_SHADER_VISIBILITY visibility, typename t_what, typename t_how, typename t_where, std::size_t slot_id>
		struct is_binding_slot<binding_slot<str_nttp, n0, visibility, t_what, t_how, t_where, slot_id>> : std::true_type
		{
		};

		template <typename t>
		inline constexpr bool is_binding_slot_v = is_binding_slot<std::remove_cvref_t<t>>::value;

		template <typename... t_binding_slot>
		consteval decltype(auto)
		find_slot_index(where::e::reg_kind kind, std::size_t register_index, std::size_t register_space = 0)
		{
			[]<auto... n>(std::index_sequence<n...>) {

			}(std::index_sequence_for<t_binding_slot...>{});
		}

		template <where::e::reg_kind kind, std::size_t register_index, std::size_t register_space = 0>
		struct find_template_param_index_pred
		{
			template <typename t>
			struct pred : std::bool_constant<
							  t::t_where::kind == kind
							  and t::t_where::register_index == register_index
							  and t::t_where::register_space == register_space>
			{
			};
		};

		template <where::e::reg_kind kind>
		struct find_slot_index_by_reg_kind_pred
		{
			template <typename t>
			struct pred : std::bool_constant<(std::to_underlying(t::t_where::kind) & std::to_underlying(kind)) != 0>
			{
			};
		};

		template <typename t_slot_type, std::size_t new_slot_id>
		struct rebind_slot;

		template <util::nttp_string_holder str_nttp, auto visibility, typename t_what, typename t_where_, std::size_t slot_id, std::size_t new_slot_id>
		struct rebind_slot<binding_slot<str_nttp, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, visibility, t_what, how::root_constant, t_where_, slot_id>, new_slot_id>
		{
			using type = binding_slot<str_nttp, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, visibility, t_what, how::root_constant, t_where_, new_slot_id>;
		};

		template <util::nttp_string_holder str_nttp, auto flags, auto visibility, typename t_what, typename t_where_, std::size_t slot_id, std::size_t new_slot_id>
		requires detail::is_root_descriptor_compatible_v<t_what>
		struct rebind_slot<binding_slot<str_nttp, flags, visibility, t_what, how::root_descriptor, t_where_, slot_id>, new_slot_id>
		{
			using type = binding_slot<str_nttp, flags, visibility, t_what, how::root_descriptor, t_where_, new_slot_id>;
		};

		template <typename t_slot, std::size_t slot_index>
		using rebind_slot_t = rebind_slot<t_slot, slot_index>::type;
	}	 // namespace detail

	template <typename... t>
	requires(detail::is_binding_slot_v<t> and ...)
	struct binding_slot_config
	{
		using t_sampler_index_seq	 = meta::filtered_index_sequence_t<detail::find_slot_index_by_reg_kind_pred<where::e::reg_kind::s>::template pred, t...>;
		using t_root_param_index_seq = meta::filtered_index_sequence_t<detail::find_slot_index_by_reg_kind_pred<static_cast<where::e::reg_kind>(7u)>::template pred, t...>;

		template <std::size_t n>
		using t_nth_binding_slot = meta::variadic_at_t<n, t...>;

		template <where::e::reg_kind kind, std::size_t register_index, std::size_t register_space = 0>
		static constexpr auto template_param_index = meta::variadic_index_v<
			detail::find_template_param_index_pred<kind, register_index, register_space>::template pred, t...>;

		template <where::e::reg_kind kind, std::size_t register_index, std::size_t register_space = 0>
		static consteval std::size_t
		calc_slot_index()
		{
			static_assert(kind == where::e::reg_kind::b or kind == where::e::reg_kind::t or kind == where::e::reg_kind::u, "Unsupported register kind");

			constexpr auto nth					= template_param_index<kind, register_index, register_space>;
			auto		   root_param_index_arr = meta::seq_to_arr(t_root_param_index_seq{});
			auto		   res					= 0;

			for (auto i : root_param_index_arr)
			{
				if (i < nth)
				{
					++res;
				}
			}

			return res;
		}

		template <std::size_t register_index, std::size_t register_space = 0>
		using reg_b =
			detail::rebind_slot_t<
				meta::variadic_at_t<template_param_index<where::e::reg_kind::b, register_index, register_space>, t...>,
				calc_slot_index<where::e::reg_kind::b, register_index, register_space>()>;

		template <std::size_t register_index, std::size_t register_space = 0>
		using reg_t =
			detail::rebind_slot_t<
				meta::variadic_at_t<template_param_index<where::e::reg_kind::t, register_index, register_space>, t...>,
				calc_slot_index<where::e::reg_kind::t, register_index, register_space>()>;

		template <std::size_t register_index, std::size_t register_space = 0>
		using reg_u =
			detail::rebind_slot_t<
				meta::variadic_at_t<template_param_index<where::e::reg_kind::u, register_index, register_space>, t...>,
				calc_slot_index<where::e::reg_kind::u, register_index, register_space>()>;

		template <typename t_binding_slot>
		requires(std::is_same_v<typename t_binding_slot::t_how, how::root_constant>)
		static inline constexpr D3D12_ROOT_PARAMETER1
		build_d3d12_root_parameter() noexcept
		{
			return D3D12_ROOT_PARAMETER1{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
				.Constants	   = {
					.ShaderRegister = t_binding_slot::t_where::register_index,
					.RegisterSpace	= t_binding_slot::t_where::register_space,
					.Num32BitValues = sizeof(t_binding_slot::t_data) / 4 },
				.ShaderVisibility = t_binding_slot::shader_visibility
			};
		}

		template <typename t_binding_slot>
		requires(std::is_same_v<typename t_binding_slot::t_how, how::root_descriptor>)
		static inline constexpr D3D12_ROOT_PARAMETER1
		build_d3d12_root_parameter() noexcept
		{
			auto param_type = D3D12_ROOT_PARAMETER_TYPE{};

			if constexpr (t_binding_slot::t_where::kind == where::e::reg_kind::b)
			{
				param_type = D3D12_ROOT_PARAMETER_TYPE_CBV;
			}
			else if constexpr (t_binding_slot::t_where::kind == where::e::reg_kind::t)
			{
				param_type = D3D12_ROOT_PARAMETER_TYPE_SRV;
			}
			else if constexpr (t_binding_slot::t_where::kind == where::e::reg_kind::u)
			{
				param_type = D3D12_ROOT_PARAMETER_TYPE_UAV;
			}
			else
			{
				static_assert(false, "Unsupported register kind");
			}

			return D3D12_ROOT_PARAMETER1{
				.ParameterType = param_type,
				.Descriptor	   = {
					.ShaderRegister = t_binding_slot::t_where::register_index,
					.RegisterSpace	= t_binding_slot::t_where::register_space,
					.Flags			= t_binding_slot::descriptor_flags },
				.ShaderVisibility = t_binding_slot::shader_visibility
			};
		}

		template <typename t_binding_slot>
		requires(std::is_same_v<typename t_binding_slot::t_how, how::static_sampler>)
		static inline constexpr D3D12_STATIC_SAMPLER_DESC1
		build_d3d12_root_parameter() noexcept
		{
			static_assert(t_binding_slot::t_where::kind == where::e::reg_kind::s, "Invalid register kind for static sampler");

			return t_binding_slot::t_what::factory(t_binding_slot::t_where::register_index,
												   t_binding_slot::t_where::register_space,
												   t_binding_slot::shader_visibility,
												   t_binding_slot::sampler_flags);
		}

		static inline root_signature::handle
		create_root_signature(D3D12_ROOT_SIGNATURE_FLAGS flags) noexcept
		{
			auto sampler_desc_arr = []<auto... i>(std::index_sequence<i...>) {
				return std::array<D3D12_STATIC_SAMPLER_DESC1, sizeof...(i)>{ build_d3d12_root_parameter<meta::variadic_at_t<i, t...>>()... };
			}(t_sampler_index_seq{});

			auto root_param_desc_arr = []<auto... i>(std::index_sequence<i...>) {
				return std::array<D3D12_ROOT_PARAMETER1, sizeof...(i)>{ build_d3d12_root_parameter<meta::variadic_at_t<i, t...>>()... };
			}(t_root_param_index_seq{});

			return root_signature::create(flags, root_param_desc_arr, sampler_desc_arr);
		};
	};
}	 // namespace age::graphics
