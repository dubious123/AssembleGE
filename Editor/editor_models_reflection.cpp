#include "pch.h"
#include "editor.h"
#include "editor_models.h"

namespace editor::models::reflection::utils
{
	namespace
	{
		struct type_info
		{
			const char* name;
			size_t		size;
		};

		const type_info _info_lut[primitive_type_count] {
			{ "int2", sizeof(int2) },
			{ "int3", sizeof(int3) },
			{ "int4", sizeof(int4) },

			{ "uint2", sizeof(uint2) },
			{ "uint3", sizeof(uint3) },
			{ "uint4", sizeof(uint4) },

			{ "float2", sizeof(float2) },
			{ "float2a", sizeof(float2a) },
			{ "float3", sizeof(float3) },
			{ "float3a", sizeof(float3a) },
			{ "float4", sizeof(float4) },
			{ "float4a", sizeof(float4a) },

			{ "float3x3", sizeof(float3x3) },
			{ "float4x4", sizeof(float4x4) },
			{ "float4x4a", sizeof(float4x4a) },

			{ "uint64", sizeof(uint64) },
			{ "uint32", sizeof(uint32) },
			{ "uint16", sizeof(uint16) },
			{ "uint8", sizeof(uint8) },

			{ "int64", sizeof(int64) },
			{ "int32", sizeof(int32) },
			{ "int16", sizeof(int16) },
			{ "int8", sizeof(int8) },

			{ "float32", sizeof(float32) },
			{ "double64", sizeof(double64) },
		};

		const std::unordered_map<std::string, e_primitive_type> _string_to_enum_map {
			{ std::string("int2"), e_primitive_type::primitive_type_int2 },
			{ std::string("int3"), e_primitive_type::primitive_type_int3 },
			{ std::string("int4"), e_primitive_type::primitive_type_int4 },

			{ std::string("uint2"), e_primitive_type::primitive_type_uint2 },
			{ std::string("uint3"), e_primitive_type::primitive_type_uint3 },
			{ std::string("uint4"), e_primitive_type::primitive_type_uint4 },

			{ std::string("float2"), e_primitive_type::primitive_type_float2 },
			{ std::string("float2a"), e_primitive_type::primitive_type_float2a },
			{ std::string("float3"), e_primitive_type::primitive_type_float3 },
			{ std::string("float3a"), e_primitive_type::primitive_type_float3a },
			{ std::string("float4"), e_primitive_type::primitive_type_float4 },
			{ std::string("float4a"), e_primitive_type::primitive_type_float4a },

			{ std::string("float3x3"), e_primitive_type::primitive_type_float3x3 },
			{ std::string("float4x4"), e_primitive_type::primitive_type_float4x4 },
			{ std::string("float4x4a"), e_primitive_type::primitive_type_float4x4a },

			{ std::string("uint64"), e_primitive_type::primitive_type_uint64 },
			{ std::string("uint32"), e_primitive_type::primitive_type_uint32 },
			{ std::string("uint16"), e_primitive_type::primitive_type_uint16 },
			{ std::string("uint8"), e_primitive_type::primitive_type_uint8 },

			{ std::string("int64"), e_primitive_type::primitive_type_int64 },
			{ std::string("int32"), e_primitive_type::primitive_type_int32 },
			{ std::string("int16"), e_primitive_type::primitive_type_int16 },
			{ std::string("int8"), e_primitive_type::primitive_type_int8 },

			{ std::string("float32"), e_primitive_type::primitive_type_float32 },
			{ std::string("double64"), e_primitive_type::primitive_type_double64 },
		};
	}	 // namespace

	size_t type_size(e_primitive_type type)
	{
		return _info_lut[type].size;
	}

	const char* type_to_string(e_primitive_type type)
	{
		return _info_lut[type].name;
	}

	e_primitive_type string_to_type(std::string str)
	{
		return _string_to_enum_map.find(str)->second;
	}

	e_primitive_type string_to_type(const char* c_str)
	{
		return string_to_type(std::string(c_str));
	}

	std::string deserialize(e_primitive_type type, const void* ptr)
	{
		switch (type)
		{
		case primitive_type_int2:
			return std::format("{}, {}", ((int2*)ptr)->x, ((int2*)ptr)->y);
		case primitive_type_int3:
			return std::format("{}, {}, {}", ((int3*)ptr)->x, ((int3*)ptr)->y, ((int3*)ptr)->z);
		case primitive_type_int4:
			return std::format("{}, {}, {}, {}", ((int4*)ptr)->x, ((int4*)ptr)->y, ((int4*)ptr)->z, ((int4*)ptr)->w);
		case primitive_type_uint2:
			return std::format("{}, {}", ((uint2*)ptr)->x, ((uint2*)ptr)->y);
		case primitive_type_uint3:
			return std::format("{}, {}, {}", ((uint3*)ptr)->x, ((uint3*)ptr)->y, ((uint3*)ptr)->z);
		case primitive_type_uint4:
			return std::format("{}, {}, {}, {}", ((uint4*)ptr)->x, ((uint4*)ptr)->y, ((uint4*)ptr)->z, ((uint4*)ptr)->w);

		case primitive_type_float2:
			return std::format("{:.5f}, {:.5f}", ((float2*)ptr)->x, ((float2*)ptr)->y);
		case primitive_type_float2a:
			return std::format("{:.5f}, {:.5f}", ((float2a*)ptr)->x, ((float2a*)ptr)->y);
		case primitive_type_float3:
			return std::format("{:.5f}, {:.5f}, {:.5f}", ((float3*)ptr)->x, ((float3*)ptr)->y, ((float3*)ptr)->z);
		case primitive_type_float3a:
			return std::format("{:.5f}, {:.5f}, {:.5f}", ((float3a*)ptr)->x, ((float3a*)ptr)->y, ((float3a*)ptr)->z);
		case primitive_type_float4:
			return std::format("{:.5f}, {:.5f}, {:.5f}, {:.5f}", ((float4*)ptr)->x, ((float4*)ptr)->y, ((float4*)ptr)->z, ((float4*)ptr)->w);
		case primitive_type_float4a:
			return std::format("{:.5f}, {:.5f}, {:.5f}, {:.5f}", ((float4a*)ptr)->x, ((float4a*)ptr)->y, ((float4a*)ptr)->z, ((float4a*)ptr)->w);

		case primitive_type_float3x3:
			return std::format("{:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}",
							   ((float3x3*)ptr)->_11, ((float3x3*)ptr)->_12, ((float3x3*)ptr)->_13,
							   ((float3x3*)ptr)->_21, ((float3x3*)ptr)->_22, ((float3x3*)ptr)->_23,
							   ((float3x3*)ptr)->_31, ((float3x3*)ptr)->_32, ((float3x3*)ptr)->_33);
		case primitive_type_float4x4:
		case primitive_type_float4x4a:
			return std::format("{:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}",
							   ((float4x4a*)ptr)->_11, ((float4x4a*)ptr)->_12, ((float4x4a*)ptr)->_13, ((float4x4a*)ptr)->_14,
							   ((float4x4a*)ptr)->_21, ((float4x4a*)ptr)->_22, ((float4x4a*)ptr)->_23, ((float4x4a*)ptr)->_24,
							   ((float4x4a*)ptr)->_31, ((float4x4a*)ptr)->_32, ((float4x4a*)ptr)->_33, ((float4x4a*)ptr)->_34,
							   ((float4x4a*)ptr)->_41, ((float4x4a*)ptr)->_42, ((float4x4a*)ptr)->_43, ((float4x4a*)ptr)->_44);
		case primitive_type_uint64:
			return std::format("{}", *(uint64*)ptr);
		case primitive_type_uint32:
			return std::format("{}", *(uint32*)ptr);
		case primitive_type_uint16:
			return std::format("{}", *(uint16*)ptr);
		case primitive_type_uint8:
			return std::format("{}", *(uint8*)ptr);
		case primitive_type_int64:
			return std::format("{}", *(int64*)ptr);
		case primitive_type_int32:
			return std::format("{}", *(int32*)ptr);
		case primitive_type_int16:
			return std::format("{}", *(int16*)ptr);
		case primitive_type_int8:
			return std::format("{}", *(int8*)ptr);
		case primitive_type_float32:
			return std::format("{}", *(float32*)ptr);
		case primitive_type_double64:
			return std::format("{}", *(double64*)ptr);
		default:
			break;
		}

		// todo nested
		assert(false);
		return {};
	}

	/// <param name="p_mem">allocated memory pointer</param>
	// void serialize(e_primitive_type type, const char* s_str, void* p_dest)
	//{
	//	serialize(type, std::string(s_str), p_dest);
	// }

	void serialize(e_primitive_type type, std::string s_str, void* p_dest)
	{
		auto tokens = editor::utilities::split_string(s_str, std::string(',', ' '));
		switch (type)
		{
		case primitive_type_int2:
		{
			new (p_dest) int2(stoi(tokens[0]), stoi(tokens[1]));
			return;
		}
		case primitive_type_int3:
		{
			new (p_dest) int3(stoi(tokens[0]), stoi(tokens[1]), stoi(tokens[2]));
			return;
		}
		case primitive_type_int4:
		{
			new (p_dest) int4(stoi(tokens[0]), stoi(tokens[1]), stoi(tokens[2]), stoi(tokens[3]));
			return;
		}
		case primitive_type_uint2:
		{
			new (p_dest) uint2(stoul(tokens[0]), stoul(tokens[1]));
			return;
		}

		case primitive_type_uint3:
		{
			new (p_dest) uint3(stoul(tokens[0]), stoul(tokens[1]), stoul(tokens[2]));
			return;
		}

		case primitive_type_uint4:
		{
			new (p_dest) uint4(stoul(tokens[0]), stoul(tokens[1]), stoul(tokens[2]), stoul(tokens[3]));
			return;
		}

		case primitive_type_float2:
		{
			new (p_dest) float2(stof(tokens[0]), stof(tokens[1]));
			return;
		}
		case primitive_type_float2a:
		{
			new (p_dest) float2a(stof(tokens[0]), stof(tokens[1]));

			return;
		}
		case primitive_type_float3:
		{
			new (p_dest) float3(stof(tokens[0]), stof(tokens[1]), stof(tokens[2]));
			return;
		}
		case primitive_type_float3a:
		{
			new (p_dest) float3a(stof(tokens[0]), stof(tokens[1]), stof(tokens[2]));
			return;
		}
		case primitive_type_float4:
		{
			new (p_dest) float4a(stof(tokens[0]), stof(tokens[1]), stof(tokens[2]), stof(tokens[3]));
			return;
		}
		case primitive_type_float4a:
		{
			new (p_dest) float4a(stof(tokens[0]), stof(tokens[1]), stof(tokens[2]), stof(tokens[3]));
			return;
		}

		case primitive_type_float3x3:
		{
			new (p_dest) float3x3(stof(tokens[0]), stof(tokens[1]), stof(tokens[2]),
								  stof(tokens[3]), stof(tokens[4]), stof(tokens[5]),
								  stof(tokens[6]), stof(tokens[7]), stof(tokens[8]));
			return;
		}
		case primitive_type_float4x4:
		case primitive_type_float4x4a:
		{
			new (p_dest) float4x4a(stof(tokens[0]), stof(tokens[1]), stof(tokens[2]), stof(tokens[3]),
								   stof(tokens[4]), stof(tokens[5]), stof(tokens[6]), stof(tokens[7]),
								   stof(tokens[8]), stof(tokens[9]), stof(tokens[10]), stof(tokens[11]),
								   stof(tokens[12]), stof(tokens[13]), stof(tokens[14]), stof(tokens[15]));
			return;
		}
		case primitive_type_uint64:
		{
			*(uint64*)p_dest = stoull(tokens[0]);
			return;
		}
		case primitive_type_uint32:
		{
			*(uint32*)p_dest = stoul(tokens[0]);
			return;
		}
		case primitive_type_uint16:
		{
			*(uint16*)p_dest = (uint16)stoul(tokens[0]);
			return;
		}
		case primitive_type_uint8:
		{
			*(uint8*)p_dest = (uint8)stoul(tokens[0]);
			return;
		}

		case primitive_type_int64:
		{
			*(int64*)p_dest = stoul(tokens[0]);
			return;
		}
		case primitive_type_int32:
		{
			*(int32*)p_dest = stoi(tokens[0]);
			return;
		}
		case primitive_type_int16:
		{
			*(int16*)p_dest = (int16)stoi(tokens[0]);
			return;
		}
		case primitive_type_int8:
		{
			*(int8*)p_dest = (int8)stoi(tokens[0]);
			return;
		}
		case primitive_type_float32:
		{
			*(float32*)p_dest = (float32)stof(tokens[0]);
			return;
		}
		case primitive_type_double64:
		{
			*(double64*)p_dest = (double64)stod(tokens[0]);
			return;
		}
		default:
			break;
		}

		// todo nested
		assert(false);
	}

	std::vector<std::string> deserialize(editor_id struct_id)
	{
		auto p_s = editor::models::reflection::find_struct(struct_id);
		return std::ranges::to<std::vector>(
			editor::models::reflection::all_fields(struct_id)
			| std::views::transform([=](auto* p_field) { return deserialize(p_field->type, p_s->p_default_value); }));
	}

	std::vector<std::string> deserialize(editor_id struct_id, const void* ptr)
	{
		auto p_s = editor::models::reflection::find_struct(struct_id);
		return std::ranges::to<std::vector>(
			editor::models::reflection::all_fields(struct_id)
			| std::views::transform([=](auto* p_field) { return deserialize(p_field->type, (char*)ptr + p_field->offset); }));
	}
}	 // namespace editor::models::reflection::utils