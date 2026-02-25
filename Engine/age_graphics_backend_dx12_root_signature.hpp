#pragma once

namespace age::graphics::root_signature::detail
{
	struct tag_b
	{
	};

	struct tag_t
	{
	};

	struct tag_u
	{
	};

	struct tag_s
	{
	};

	template <typename t_tag>
	struct shader_register
	{
		using t_tag_type = t_tag;

		uint32 idx	 = 0;
		uint32 space = 0;
	};

	template <typename t_tag>
	consteval D3D12_ROOT_PARAMETER_TYPE
	d3d12_root_parameter_type()
	{
		if constexpr (std::is_same_v<t_tag, tag_b>)
		{
			return D3D12_ROOT_PARAMETER_TYPE_CBV;
		}
		else if constexpr (std::is_same_v<t_tag, tag_t>)
		{
			return D3D12_ROOT_PARAMETER_TYPE_SRV;
		}
		else if constexpr (std::is_same_v<t_tag, tag_u>)
		{
			return D3D12_ROOT_PARAMETER_TYPE_UAV;
		}
		else if constexpr (std::is_same_v<t_tag, tag_s>)
		{
			static_assert(false, "invalid t_tag");
		}
		else
		{
			static_assert(false, "invalid t_tag");
		}
	}

	template <typename t_tag>
	consteval D3D12_DESCRIPTOR_RANGE_TYPE
	d3d12_descriptor_range_type()
	{
		if constexpr (std::is_same_v<t_tag, tag_b>)
		{
			return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		}
		else if constexpr (std::is_same_v<t_tag, tag_t>)
		{
			return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		}
		else if constexpr (std::is_same_v<t_tag, tag_u>)
		{
			return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		}
		else if constexpr (std::is_same_v<t_tag, tag_s>)
		{
			return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		}
		else
		{
			static_assert(false, "invalid t_tag");
		}
	}
}	 // namespace age::graphics::root_signature::detail

namespace age::graphics::root_signature
{
	using b = detail::shader_register<detail::tag_b>;
	using t = detail::shader_register<detail::tag_t>;
	using u = detail::shader_register<detail::tag_u>;
	using s = detail::shader_register<detail::tag_s>;
}	 // namespace age::graphics::root_signature

namespace age::graphics::root_signature
{
	struct constants
	{
		detail::shader_register<detail::tag_b> reg		  = b{ .idx = 0, .space = 0 };
		uint32								   num_32bit  = 0;
		D3D12_SHADER_VISIBILITY				   visibility = D3D12_SHADER_VISIBILITY_ALL;

		constexpr D3D12_ROOT_PARAMETER1
		build_d3d12_root_parameter() const noexcept
		{
			return D3D12_ROOT_PARAMETER1{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
				.Constants	   = {
					.ShaderRegister = reg.idx,
					.RegisterSpace	= reg.space,
					.Num32BitValues = num_32bit },
				.ShaderVisibility = visibility
			};
		}
	};

	template <typename t>
	concept cx_constants = std::is_same_v<std::remove_cvref_t<t>, constants>;
}	 // namespace age::graphics::root_signature

namespace age::graphics::root_signature
{
	template <typename t_reg>
	struct descriptor
	{
		static_assert(std::is_same_v<t_reg, b> or std::is_same_v<t_reg, t> or std::is_same_v<t_reg, u>,
					  "invalid t_reg");

		t_reg						reg;
		D3D12_ROOT_DESCRIPTOR_FLAGS flags	   = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
		D3D12_SHADER_VISIBILITY		visibility = D3D12_SHADER_VISIBILITY_ALL;

		static constexpr D3D12_ROOT_PARAMETER_TYPE
		root_param_type()
		{
			return detail::d3d12_root_parameter_type<typename t_reg::t_tag_type>();
		}

		constexpr D3D12_ROOT_PARAMETER1
		build_d3d12_root_parameter() const noexcept
		{
			return D3D12_ROOT_PARAMETER1{
				.ParameterType = root_param_type(),
				.Descriptor	   = {
					.ShaderRegister = reg.idx,
					.RegisterSpace	= reg.space,
					.Flags			= flags },
				.ShaderVisibility = visibility
			};
		}
	};

	template <typename>
	struct is_descriptor : std::false_type
	{
	};

	template <typename t_reg>
	struct is_descriptor<descriptor<t_reg>> : std::true_type
	{
	};

	template <typename t>
	concept cx_descriptor = is_descriptor<std::remove_cvref_t<t>>::value;
}	 // namespace age::graphics::root_signature

namespace age::graphics::root_signature
{
	template <typename t_reg>
	struct descriptor_range
	{
		static_assert(std::is_same_v<t_reg, b> or std::is_same_v<t_reg, t> or std::is_same_v<t_reg, u> or std::is_same_v<t_reg, s>,
					  "invalid t_reg");

		using t_shader_register = t_reg;

		t_reg  reg;
		uint32 count;

		D3D12_DESCRIPTOR_RANGE_FLAGS flags	= D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		uint32						 offset = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		static constexpr D3D12_DESCRIPTOR_RANGE_TYPE
		desc_range_type()
		{
			return detail::d3d12_descriptor_range_type<typename t_reg::t_tag_type>();
		}

		constexpr D3D12_DESCRIPTOR_RANGE1
		build_d3d12_descriptor_range() const noexcept
		{
			return D3D12_DESCRIPTOR_RANGE1{
				.RangeType						   = desc_range_type(),
				.NumDescriptors					   = count,
				.BaseShaderRegister				   = reg.idx,
				.RegisterSpace					   = reg.space,
				.Flags							   = flags,
				.OffsetInDescriptorsFromTableStart = offset
			};
		}
	};

	template <typename>
	struct is_descriptor_range : std::false_type
	{
	};

	template <typename t_reg>
	struct is_descriptor_range<descriptor_range<t_reg>> : std::true_type
	{
	};

	template <typename t>
	concept cx_descriptor_range = is_descriptor_range<std::remove_cvref_t<t>>::value;
}	 // namespace age::graphics::root_signature

namespace age::graphics::root_signature
{
	template <typename... t_desc_range>
	struct descriptor_table
	{
		D3D12_SHADER_VISIBILITY										 visibility;
		std::array<D3D12_DESCRIPTOR_RANGE1, sizeof...(t_desc_range)> descriptor_range_arr;

		static constexpr auto sampler_reg_count = age::meta::filter_count<age::meta::pred_is_same<s>::template type, typename std::remove_cvref_t<t_desc_range>::t_shader_register...>();
		static_assert(
			(sampler_reg_count == 0) or (sampler_reg_count == sizeof...(t_desc_range)),
			"descriptor_table must be either sampler-only or resource-only. "
			"Do not mix sampler (s#) ranges with CBV/SRV/UAV ranges.");

		constexpr descriptor_table(D3D12_SHADER_VISIBILITY visibility, auto&&... arg) noexcept
			: visibility{ visibility },
			  descriptor_range_arr{ (FWD(arg).build_d3d12_descriptor_range())... }
		{
		}

		constexpr D3D12_ROOT_PARAMETER1
		build_d3d12_root_parameter() const noexcept
		{
			return D3D12_ROOT_PARAMETER1{
				.ParameterType	 = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable = {
					.NumDescriptorRanges = sizeof...(t_desc_range),
					.pDescriptorRanges	 = descriptor_range_arr.data() },
				.ShaderVisibility = visibility
			};
		}
	};

	template <typename...>
	struct is_descriptor_table : std::false_type
	{
	};

	template <typename... t_desc_range>
	struct is_descriptor_table<descriptor_table<t_desc_range...>> : std::true_type
	{
	};

	template <typename t>
	concept cx_descriptor_table = is_descriptor_table<std::remove_cvref_t<t>>::value;
}	 // namespace age::graphics::root_signature

namespace age::graphics::root_signature::detail
{
	template <typename t>
	concept cx_root_parameter = cx_constants<t> or cx_descriptor<t> or cx_descriptor_table<t>;
}

namespace age::graphics::root_signature
{
	inline void
	init() noexcept
	{
		auto feature = D3D12_FEATURE_DATA_ROOT_SIGNATURE{
			.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_2
		};

		AGE_HR_CHECK(g::p_main_device->CheckFeatureSupport(
			D3D12_FEATURE_ROOT_SIGNATURE,
			&feature,
			sizeof(feature)));
	}

	inline void
	deinit() noexcept
	{
		for (auto* p_root_signature : g::root_signature_ptr_vec)
		{
			p_root_signature->Release();
		}

		if constexpr (age::config::debug_mode)
		{
			g::root_signature_ptr_vec.debug_validate();
		}

		g::root_signature_ptr_vec.clear();
	}
}	 // namespace age::graphics::root_signature

namespace age::graphics::root_signature
{
	template <auto root_param_size, auto sampler_param_size>
	handle
	create(D3D12_ROOT_SIGNATURE_FLAGS										 flags,
		   const std::array<D3D12_ROOT_PARAMETER1, root_param_size>&		 root_param_arr,
		   const std::array<D3D12_STATIC_SAMPLER_DESC1, sampler_param_size>& sampler_arr) noexcept
	{
		auto root_signature_desc = D3D12_VERSIONED_ROOT_SIGNATURE_DESC{
			.Version  = D3D_ROOT_SIGNATURE_VERSION_1_2,
			.Desc_1_2 = {
				.NumParameters	   = static_cast<UINT>(root_param_arr.size()),
				.pParameters	   = root_param_arr.data(),
				.NumStaticSamplers = static_cast<UINT>(sampler_arr.size()),
				.pStaticSamplers   = sampler_arr.data(),
				.Flags			   = flags }
		};

		auto* p_serialized_root_signature = (ID3DBlob*)nullptr;
		{
			auto* p_error_blob = (ID3DBlob*)nullptr;

			auto hr = ::D3D12SerializeVersionedRootSignature(
				&root_signature_desc,
				&p_serialized_root_signature,
				&p_error_blob);

			if (p_error_blob is_not_nullptr)
			{
				std::println("{}", static_cast<const char*>(p_error_blob->GetBufferPointer()));
				p_error_blob->Release();
			}

			AGE_HR_CHECK(hr);
		}

		auto* p_signature = (ID3D12RootSignature*)nullptr;

		AGE_HR_CHECK(
			g::p_main_device->CreateRootSignature(
				0,
				p_serialized_root_signature->GetBufferPointer(),
				p_serialized_root_signature->GetBufferSize(),
				IID_PPV_ARGS(&p_signature)));

		p_serialized_root_signature->Release();

		return handle{ .id = g::root_signature_ptr_vec.emplace_back(p_signature) };
	}

	handle
	create(D3D12_ROOT_SIGNATURE_FLAGS flags, auto&&... root_parameter) noexcept
		requires(detail::cx_root_parameter<BARE_OF(root_parameter)> and ...)
	{
		static_assert((detail::cx_root_parameter<decltype(root_parameter)> and ...),
					  "invalid root parameter: expected constants/descriptor/descriptor_table");

		return root_signature::create(flags,
									  std::array<D3D12_ROOT_PARAMETER1, sizeof...(root_parameter)>{ root_parameter.build_d3d12_root_parameter()... },
									  std::array<D3D12_STATIC_SAMPLER_DESC1, 0>{});
	}

	inline void
	destroy(handle h) noexcept
	{
		g::root_signature_ptr_vec[h.id]->Release();
		g::root_signature_ptr_vec.remove(h.id);
	}
}	 // namespace age::graphics::root_signature
