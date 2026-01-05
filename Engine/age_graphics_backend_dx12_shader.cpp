#include "age_pch.hpp"
#include "age.hpp"

namespace age::graphics
{
	// add dxil.dll and dxcompiler.dll
	D3D12_SHADER_BYTECODE
	compile_shader(
		std::wstring_view file_name,
		std::wstring_view entry_point,
		std::wstring_view target) noexcept
	{
		auto* p_utils			= (IDxcUtils*)nullptr;
		auto* p_compiler		= (IDxcCompiler3*)nullptr;
		auto* p_file			= (IDxcBlobEncoding*)nullptr;
		auto* p_include_handler = (IDxcIncludeHandler*)nullptr;
		auto* p_result			= (IDxcResult*)nullptr;
		auto* p_res_blob		= (IDxcBlob*)nullptr;

		AGE_HR_CHECK(::DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&p_utils)));
		AGE_HR_CHECK(::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&p_compiler)));
		AGE_HR_CHECK(p_utils->LoadFile(file_name.data(), nullptr, &p_file));
		AGE_HR_CHECK(p_utils->CreateDefaultIncludeHandler(&p_include_handler));

		{
			auto buffer = DxcBuffer{
				.Ptr	  = p_file->GetBufferPointer(),
				.Size	  = p_file->GetBufferSize(),
				.Encoding = DXC_CP_UTF8
			};

			auto args = [=] {
				if constexpr (age::config::debug_mode)
				{
					return std::array{
						file_name.data(),
						L"-E",
						entry_point.data(),
						L"-T",
						target.data(),
						L"-Zi",
						L"-Od"
					};
				}
				else
				{
					return std::array{
						file_name.data(),
						L"-E",
						entry_point.data(),
						L"-T",
						target.data(),
						L"-E",
						L"-T"
					};
				}
			}();

			AGE_HR_CHECK(p_compiler->Compile(
				&buffer,
				args.data(),
				static_cast<UINT32>(args.size()),
				p_include_handler,
				IID_PPV_ARGS(&p_result)));
		}

		{
			auto* p_error = (IDxcBlobUtf8*)nullptr;

#pragma warning(disable : 6387)
			p_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&p_error), nullptr);
#pragma warning(default : 6387)

			if (p_error is_not_nullptr)
			{
				std::println("{}", p_error->GetStringPointer());

				p_error->Release();
			}

			auto status = (HRESULT)S_OK;
			p_result->GetStatus(&status);
			AGE_HR_CHECK(status);
		}

		AGE_HR_CHECK(p_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&p_res_blob), nullptr));

		{
			p_utils->Release();
			p_compiler->Release();
			p_file->Release();
			p_include_handler->Release();
			p_result->Release();
		}

		return D3D12_SHADER_BYTECODE{
			.pShaderBytecode = p_res_blob->GetBufferPointer(),
			.BytecodeLength	 = p_res_blob->GetBufferSize()
		};
	}
}	 // namespace age::graphics