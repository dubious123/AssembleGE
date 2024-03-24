#pragma once
#include "__common.h"
#include "__meta.h"
#include "__ecs.h"

#define OFFSET(struct_name, field_name) ((size_t) & (((__detail__struct_name*)(nullptr))->field_name))

#define COMPONENT_BEGIN(struct_name)                                           \
	struct struct_name                                                         \
	{                                                                          \
	  public:                                                                  \
		static constinit const uint64 id = STR_HASH(#struct_name);             \
                                                                               \
                                                                               \
	  private:                                                                 \
		typedef struct_name		  __detail_self;                               \
		static inline const char* __detail__struct_name = []() {               \
			/*reflection::register_struct(id, #struct_name);               \*/ \
			reflection::register_struct(#struct_name, id);                     \
			return #struct_name;                                               \
		}();                                                                   \
                                                                               \
	  public:


#define SERIALIZE_FIELD(type, field_name, ...)                                                                                                                          \
	type field_name { __VA_ARGS__ };                                                                                                                                    \
                                                                                                                                                                        \
  private:                                                                                                                                                              \
	static inline bool __detail__field_init_##field_name = []() {                                                                                                       \
		/*reflection::register_field(id, __detail__struct_name, #type, #field_name, field_offset<__detail_self, type, &__detail_self::field_name>(), #__VA_ARGS__); \*/ \
		reflection::register_field(__detail__struct_name, #type, #field_name, field_offset<__detail_self, type, &__detail_self::field_name>(), #__VA_ARGS__);           \
		return false;                                                                                                                                                   \
	}();                                                                                                                                                                \
                                                                                                                                                                        \
  public:
#define COMPNENT_END() \
	}                  \
	;

#define SERIALIZE_WORLD(world_name, ...) reflection::world_wrapper<#world_name, __VA_ARGS__>

#define SERIALIZE_SCENE(scene_name, ...) static inline auto scene_name = [] { \
	reflection::scene_wrapper<#scene_name,__VA_ARGS__>();  \
	return reflection::scene_wrapper< #scene_name,__VA_ARGS__ >::scene_type(); }();

template <typename _Tstruct, typename _Tfield, typename _Tfield _Tstruct::*Field>
static size_t field_offset()
{
	return (size_t) & ((_Tstruct*)(0)->*Field);
}

namespace reflection
{
	struct field_info
	{
		const char* name;
		const char* type;
		const char* serialized_value;
		size_t		offset;
		// std::string				child_count;
		// std::vector<field_info> childs;
	};

	struct struct_info
	{
		struct_info();
		struct_info(uint64 idx, const char* name);
		struct_info(uint64 idx, uint64 hash_id, const char* name);
		uint64		idx;
		uint64		hash_id;
		const char* name;
		size_t		field_count;
		field_info* fields;
	};

	struct scene_info
	{
		const char* name;
		size_t		world_count;
		size_t		world_idx;
	};

	struct world_info
	{
		const char* name;
		size_t		scene_idx;
		uint64		struct_count;
		uint64		struct_idx_vec[64];
	};

	struct component_info
	{
		uint64 struct_idx;
		size_t scene_idx;
		size_t world_idx;
	};

	void register_struct(const char* name, uint64 hash_id);

	void register_field(const char* struct_name, const char* type, const char* name, size_t offset, const char* serialized_value);

	void register_scene(const char* name);

	void register_world(const char* name);

	void register_component_to_world(uint64 struct_hash_id);

	EDITOR_API size_t get_registered_struct_count();
	EDITOR_API size_t get_registered_scene_count();
	EDITOR_API size_t get_registered_world_count();

	EDITOR_API struct_info*	   get_struct_info(uint64 component_id);
	EDITOR_API scene_info*	   get_scene_info(size_t index);
	EDITOR_API world_info*	   get_world_info(size_t index);
	EDITOR_API component_info* get_component_info(size_t index);
}	 // namespace reflection

namespace reflection
{
	template <meta::string_wrapper str_wrapper, typename... c>
	struct world_wrapper
	{
		using world_type = ecs::world<c...>;

		world_wrapper()
		{
			reflection::register_world(str_wrapper.value);
			([] {
				reflection::register_component_to_world(c::id);
			}(),
			 ...);
		}
	};

	template <meta::string_wrapper str_wrapper, typename... w>
	struct scene_wrapper
	{
		using scene_type = ecs::scene<typename w::world_type...>;

		scene_wrapper()
		{
			reflection::register_scene(str_wrapper.value);
			([] {
				w();
			}(),
			 ...);
		}
	};
}	 // namespace reflection