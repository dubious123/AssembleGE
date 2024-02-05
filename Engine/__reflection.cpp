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

		data_structure::vector<world_info>& _world_info_vec()
		{
			static data_structure::vector<world_info> _vec;
			return _vec;
		}

		data_structure::vector<component_info>& _component_info_vec()
		{
			static data_structure::vector<component_info> _vec;
			return _vec;
		}
	}	 // namespace

	struct_info::struct_info()
	{
		this->idx		  = -1;
		this->name		  = nullptr;
		this->fields	  = nullptr;
		this->field_count = 0;
	}

	struct_info::struct_info(uint64 idx, const char* name)
	{
		this->idx		  = idx;
		this->name		  = name;
		this->fields	  = nullptr;
		this->field_count = 0;
	}

	void register_struct(const char* name)
	{
		_struct_info_vec().emplace_back(_struct_info_vec().size(), name);
		_field_info_vec().emplace_back();
	}

	void reflection::register_field(const char* struct_name, const char* type, const char* name, size_t offset, const char* serialized_value)
	{
		auto info			  = field_info();
		info.name			  = name;
		info.type			  = type;
		info.offset			  = offset;
		info.serialized_value = serialized_value;

		auto& field_vec = _field_info_vec().back();
		field_vec.emplace_back(info);
		_struct_info_vec().back().fields	  = &field_vec[0];
		_struct_info_vec().back().field_count = field_vec.size();
	}

	void reflection::register_scene(const char* scene_name)
	{
		_scene_info_vec().emplace_back(scene_name, 0ui64, _world_info_vec().size());
	}

	void reflection::register_world(const char* world_name)
	{
		_world_info_vec().emplace_back(world_name, _scene_info_vec().size() - 1, 0ul, 0ul);
		auto& scene_info = _scene_info_vec().back();

		++scene_info.world_count;
	}

	void register_component_to_world(uint64 struct_idx)
	{
		auto& world_info = _world_info_vec().back();

		if (world_info.component_count == 0)
		{
			world_info.component_idx = _component_info_vec().size();
		}

		++world_info.component_count;
		_component_info_vec().emplace_back(struct_idx, world_info.scene_idx, _world_info_vec().size() - 1);
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

	component_info* get_component_info(size_t index)
	{
		auto& res = _component_info_vec()[index];
		return &res;
	}
}	 // namespace reflection
