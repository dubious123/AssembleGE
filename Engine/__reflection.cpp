#include "__reflection.h"

namespace reflection
{
	namespace
	{
		// map => vector + algorithm
		data_structure::vector<struct_info>& _struct_info_vec()
		{
			static data_structure::vector<struct_info> _vec;
			return _vec;
		}

		data_structure::vector<data_structure::vector<field_info>>& _field_info_vec()
		{
			static data_structure::vector<data_structure::vector<field_info>> _vec;
			return _vec;
		}

		data_structure::vector<scene_info>& _scene_info_vec()
		{
			static data_structure::vector<scene_info> _vec;
			return _vec;
		}

		data_structure::vector<ecs::world_base*>& _world_base_vec()
		{
			static data_structure::vector<ecs::world_base*> _vec;
			return _vec;
		}

		data_structure::vector<world_info>& _world_info_vec()
		{
			static data_structure::vector<world_info> _vec;
			return _vec;
		}

		data_structure::vector<data_structure::vector<entity_info>>& _entity_info_vec()
		{
			static data_structure::vector<data_structure::vector<entity_info>> _vec;
			return _vec;
		}

		data_structure::vector<system_info>& _system_info_vec()
		{
			static data_structure::vector<system_info> _vec;
			return _vec;
		}
	}	 // namespace

	struct_info::struct_info(uint64 idx, uint64 hash_id, const char* name, void* p_value)
		: idx(idx), hash_id(hash_id), name(name), p_default_value(p_value), fields(nullptr), field_count(0)
	{
	}

	struct_info::struct_info(struct_info&& other) noexcept
		: idx(other.idx), hash_id(other.hash_id), name(other.name), p_default_value(other.p_default_value), fields(other.fields), field_count(other.field_count)
	{
		other.p_default_value = nullptr;
	}

	struct_info& struct_info::operator=(struct_info&& other) noexcept
	{
		assert(p_default_value is_not_nullptr);
		free(p_default_value);

		idx				= other.idx;
		hash_id			= other.hash_id;
		name			= other.name;
		p_default_value = other.p_default_value;
		fields			= other.fields;
		field_count		= other.field_count;

		other.p_default_value = nullptr;
		return *this;
	}

	struct_info::~struct_info()
	{
		free(p_default_value);
	}

	world_info::world_info(const char* name, uint64 idx, size_t s_count) : name(name), scene_idx(idx), struct_count(s_count) { }

	void register_struct(const char* name, uint64 hash_id, void* p_value)
	{
		_struct_info_vec().emplace_back(_struct_info_vec().size(), hash_id, name, p_value);
		_field_info_vec().emplace_back();
	}

	void register_field(e_primitive_type type, const char* name, size_t offset)
	{
		auto info	 = field_info();
		info.name	 = name;
		info.type	 = type;
		info.offset	 = offset;
		info.p_value = nullptr;

		auto& field_vec = _field_info_vec().back();
		field_vec.emplace_back(info);
		auto& struct_info					  = _struct_info_vec().back();
		_struct_info_vec().back().fields	  = &field_vec[0];
		_struct_info_vec().back().field_count = field_vec.size();
	}

	void register_scene(const char* scene_name)
	{
		_scene_info_vec().emplace_back(scene_name, 0ui64, _world_info_vec().size());
	}

	void register_world(const char* world_name, const ecs::world_base& w)
	{
		// todo register world base
		_world_base_vec().emplace_back(&(ecs::world_base&)w);
		_world_info_vec().emplace_back(world_name, _scene_info_vec().size() - 1, 0ul);
		_entity_info_vec().emplace_back();
		auto& scene_info = _scene_info_vec().back();

		++scene_info.world_count;
	}

	void register_component_to_world(uint64 struct_hash_id)
	{
		auto& world_info = _world_info_vec().back();

		for (auto i = 0; i < _struct_info_vec().size(); ++i)
		{
			auto& struct_info = _struct_info_vec()[i];
			if (struct_info.hash_id == struct_hash_id)
			{
				world_info.struct_idx_vec[world_info.struct_count++] = i;
				break;
			}
		}
	}

	void register_entity(const char* entity_name, ecs::entity_idx e_idx, const ecs::world_base& w)
	{
		auto w_idx = _world_info_vec().size() - 1;
		assert(_entity_info_vec().size() > w_idx);

		auto& e_info	 = _entity_info_vec()[w_idx].emplace_back();
		e_info.name		 = entity_name;
		e_info.idx		 = e_idx;
		e_info.archetype = w.entities[e_idx].archetype;
		// _world_base_vec().emplace_back(&(ecs::world_base&)w);
	}

	void register_system_begin(const char* system_name)
	{
		_system_info_vec().push_back({ system_name, {} });
	}

	void register_system_function(int type, int param_type)
	{
		// register_system("hello");
		_system_info_vec().back().interfaces[type * 2]	   = 1;
		_system_info_vec().back().interfaces[type * 2 + 1] = param_type;
	}

	void register_system_update(uint32 count, uint64* (*alloc_func)())
	{
		_system_info_vec().back().update_argument_count = count;
		_system_info_vec().back().p_arguments			= alloc_func();
	}

	size_t get_registered_struct_count()
	{
		return _struct_info_vec().size();
	}

	size_t get_registered_scene_count()
	{
		return _scene_info_vec().size();
	}

	size_t get_registered_world_count()
	{
		return _world_info_vec().size();
	}

	size_t get_registered_entity_count(size_t world_idx)
	{
		return _entity_info_vec()[world_idx].size();
	}

	size_t get_registered_system_count()
	{
		return _system_info_vec().size();
	}

	struct_info* get_struct_info(uint64 struct_idx)
	{
		auto& res = _struct_info_vec()[struct_idx];
		return &_struct_info_vec()[struct_idx];
	}

	scene_info* get_scene_info(size_t index)
	{
		auto& res = _scene_info_vec()[index];
		return &res;
	}

	world_info* get_world_info(size_t index)
	{
		auto& res = _world_info_vec()[index];
		return &res;
	}

	component_info get_component_info(size_t world_idx, size_t entity_idx, size_t component_idx)
	{
		entity_info* e_info		  = get_entity_info(world_idx, entity_idx);
		auto*		 p_world_base = _world_base_vec()[world_idx];
		auto&		 e			  = p_world_base->entities[e_info->idx];

		auto& mem_block = p_world_base->memory_block_vec_map[e.archetype][e.mem_block_idx];
		return { mem_block.get_component_size(component_idx), mem_block.get_component_ptr(e.memory_idx, component_idx) };
	}

	entity_info* get_entity_info(size_t world_idx, size_t entity_idx)
	{
		return &_entity_info_vec()[world_idx][entity_idx];
	}

	system_info* get_system_info(size_t system_idx)
	{
		return &_system_info_vec()[system_idx];
	}
}	 // namespace reflection
