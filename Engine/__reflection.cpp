#include "__reflection.h"

namespace reflection
{
	namespace
	{
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
	}	 // namespace

	struct_info::struct_info(const char* name)
	{
		this->name		  = name;
		this->fields	  = nullptr;
		this->field_count = 0;
	}

	void register_struct(const char* name)
	{
		_struct_info_vec().emplace_back(name);
		_field_info_vec().emplace_back();

		// return _struct_info_vec().size() - 1;
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
		_scene_info_vec().emplace_back(scene_name, _world_info_vec().size(), 0ui64);
	}

	void reflection::register_world(const char* world_name)
	{
		_world_info_vec().emplace_back(_scene_info_vec().size() - 1, world_name);
		auto& scene_info = _scene_info_vec().back();
		if (scene_info.world_count == 0)
		{
			scene_info.world_idx = _world_info_vec().size() - 1;
		}

		++scene_info.world_count;
		auto& world_info = _world_info_vec().back();
	}

	size_t get_registered_component_count()
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

	struct_info* get_component_info(size_t index)
	{
		auto& res = _struct_info_vec()[index];
		return &_struct_info_vec()[index];
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
}	 // namespace reflection
