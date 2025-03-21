#include "pch.h"
#include "Editor.h"
#include "editor_style.h"
#include "editor_widgets.h"
#include "platform.h"
#include "editor_view.h"
#include "editor_models.h"
#include "editor_ctx_item.h"
#include "game.h"

namespace editor::view::hierarchy
{
	auto _open = true;

	const auto cmd_open = editor_command {
		"Open Hierarchy Window",
		ImGuiKey_None,
		[](editor_id _) {
			return true;
		},
		[](editor_id _) {
			_open ^= true;
		}
	};

	void show()
	{
		using namespace std::ranges;

		if (_open is_false) return;
		if (editor::game::get_pproject()->is_opened is_false) return;

		if (widgets::begin("Hierarchy", &_open))
		{
			// static auto selection_mask = 2;
			// static char buffer[SCENE_NAME_MAX_LEN];
			// static auto rename_scene = false;
			// auto		node_clicked = -1;
			// auto* p_project = editor::game::get_current_p_project();

			widgets::set_cursor_pos_x((platform::get_window_size().x - platform::get_window_info().default_item_width()) * 0.5f);
			auto p_s_current = models::scene::get_current();

			if (p_s_current == nullptr)
			{
				widgets::end();
				return;
			}

			// todo widget begin combo
			if (ImGui::BeginCombo("##scene_combo", p_s_current->name.c_str(), ImGuiComboFlags_NoArrowButton))
			{
				auto current_id = models::scene::get_current();
				for (const auto* p_scene : models::scene::all())
				{
					auto is_current = p_scene->id == p_s_current->id;
					if (widgets::selectable(p_scene->name, is_current))
					{
						models::scene::cmd_set_current(p_scene->id);
						editor::select_new(p_scene->id);
					}

					if (is_current)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}


			widgets::separator();

			if (widgets::tree_node(p_s_current->name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | (editor::is_selected(p_s_current->id) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None)))
			{
				editor::add_left_right_click_source(p_s_current->id);

				for (const auto* p_w : models::world::all(p_s_current->id))
				{
					auto world_selected			= editor::is_selected(p_w->id);
					auto world_tree_node_opened = widgets::tree_node(p_w->name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | (editor::is_selected(p_w->id) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None));

					editor::add_left_right_click_source(p_w->id);

					if (world_tree_node_opened is_false) continue;

					std::ranges::for_each(models::entity::all(p_w->id), [](const auto* p_e) {
						ImGui::TreeNodeEx(p_e->name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (editor::is_selected(p_e->id) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None));
						editor::add_left_right_click_source(p_e->id);
					});

					// if (p_w->entities.empty() is_false)
					//{
					//	widgets::separator();
					// }

					// for (const auto& system_id : p_w->systems)
					//{
					//	auto p_system = models::system::find(system_id);
					//	ImGui::TreeNodeEx(p_system->name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (editor::is_selected(system_id) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None));
					//	editor::selection_source(system_id);
					//	editor::ctx_source(system_id);
					// }

					// if (p_w->systems.empty() is_false)
					//{
					//	widgets::separator();
					// }

					// Widgets::Separator();

					widgets::tree_pop();

					// for (const auto& sub_world_id : p_world->SubWorlds)
					//{
					//	auto p_sub_world = Models::World::Find(sub_world_id);
					//	if (ImGui::TreeNode(p_sub_world->Name.c_str()))
					//	{
					//		ImGui::TreePop();
					//	}
					// }
				}

				widgets::tree_pop();
			}


			// if (node_clicked != -1)
			//{
			//	// Update selection state
			//	// (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
			//	if (ImGui::GetIO().KeyCtrl)
			//		selection_mask ^= (1 << node_clicked);	  // CTRL+click to toggle
			//	else										  // if (!(selection_mask & (1 << node_clicked))) // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
			//		selection_mask = (1 << node_clicked);	  // Click to single-select
			// }
		}

		widgets::end();
	}
}	 // namespace editor::view::hierarchy

namespace editor::view::inspector
{
	using namespace editor::models;
	auto _open = true;

	const auto cmd_open = editor_command {
		"Open Inspector Window",
		ImGuiKey_None,
		[](editor_id _) {
			return true;
		},
		[](editor_id _) {
			_open ^= true;
		}
	};

	void* _offset_to_ptr(size_t offset, const void* ptr)
	{
		return (void*)((char*)ptr + offset);
	}

	void _draw_struct(editor_id struct_id, const void* ptr)
	{
		auto* p_s = editor::models::reflection::find_struct(struct_id);
		// auto& str_vec = editor::models::reflection::utils::deserialize(struct_id, ptr);
		if (widgets::tree_node(p_s->name))
		{
			std::ranges::for_each(editor::models::reflection::all_fields(p_s->id), [=](em_field* p_f) {
				auto selected = false;
				widgets::tree_node(std::format("{} ({}) : {}", p_f->name, editor::models::reflection::utils::type_to_string(p_f->type), editor::models::reflection::utils::deserialize(p_f->type, _offset_to_ptr(p_f->offset, ptr))), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);

				// widgets::component_drag(std::format("##{}", p_f->id.value), p_f->type, _offset_to_ptr(p_f->offset, ptr));

				if (widgets::is_item_activated())
				{
					editor::logger::info("start value : {:f}", *(float*)(_offset_to_ptr(p_f->offset, ptr)));
				}
				else if (widgets::is_item_deactivated_after_edit())
				{
					editor::logger::info("end value : {:f}", *(float*)(_offset_to_ptr(p_f->offset, ptr)));
				}
			});

			widgets::tree_pop();
		}
	}

	void _draw_struct(editor_id struct_id)
	{
		auto* p_s = editor::models::reflection::find_struct(struct_id);
		// auto& str_vec = editor::models::reflection::utils::deserialize(struct_id, ptr);
		if (widgets::tree_node(p_s->name.c_str()))
		{
			std::ranges::for_each(editor::models::reflection::all_fields(p_s->id), [=](em_field* p_f) {
				auto selected = false;
				widgets::tree_node(std::format("{} ({}) : {}",
											   p_f->name,
											   models::reflection::utils::type_to_string(p_f->type),
											   models::reflection::utils::deserialize(p_f->type, game::ecs::get_field_pvalue(p_s->ecs_idx, p_f->ecs_idx))),
								   ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			});

			widgets::tree_pop();
		}
	}

	void _draw_scene(editor_id _)
	{
		auto p_scene = scene::get_current();
		widgets::editable_header(p_scene->id, p_scene->name);

		widgets::text("scene idx : {%d}", p_scene->id);
		// widgets::text("world count : {%d}", p_scene->world_count());
	}

	void _draw_world(editor_id world_id)
	{
		auto p_world = world::find(world_id);
		if (p_world is_nullptr) return;

		auto p_scene = scene::find(p_world->scene_id);

		widgets::editable_header(world_id, p_world->name);

		if (widgets::tree_node("archetype", ImGuiTreeNodeFlags_DefaultOpen))
		{
			std::ranges::for_each(p_world->structs, [](editor_id struct_id) {
				_draw_struct(struct_id);
				// auto* p_struct = models::reflection::find_struct(struct_id);
				// if (widgets::tree_node(p_struct->name))
				//{
				//	std::ranges::for_each(models::reflection::all_fields(p_struct->id), [](em_field* p_f) {
				//		widgets::tree_node(std::format("{}({}) : {}", p_f->name, models::reflection::utils::type_to_string(p_f->type), p_f->serialized_value).c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
				//	});
				//	widgets::tree_pop();
				// }
			});
			widgets::tree_pop();
		}


		// for (auto c = 0; c < p_world->p_info->component_count; ++c)
		//{
		//	auto p_c_info = p_world->find_component(p_world->p_info->component_idx + c);
		//	auto p_s_info = reflection::find_struct(p_c_info->struct_id);
		//	widgets::text(p_s_info->name);

		//	for (auto f = 0; f < p_s_info->field_count; ++f)
		//	{
		//		auto p_f_info = (p_s_info->fields + f);
		//		widgets::text(p_f_info->name);
		//		widgets::text(p_f_info->type);
		//		widgets::text(p_f_info->serialized_value);
		//	}
		//}

		// widgets::separator();

		// if (ImGui::CollapsingHeader("Systems"))
		//{
		//	for (auto system_id : p_world->systems)
		//	{
		//		char buffer[100] {};
		//		auto p_system = system::find(system_id);
		//		std::copy_n(p_system->name.begin(), p_system->name.size(), buffer);
		//		if (ImGui::InputText(std::format("##{}", system_id.ToString()).c_str(), buffer, 100, ImGuiInputTextFlags_AutoSelectAll))
		//		{
		//			p_system->name = std::string(buffer);
		//		}
		//	}
		// }

		// if (ImGui::CollapsingHeader("Subworlds"))
		//{
		//	for (auto subworld_id : p_world->subworlds)
		//	{
		//		auto p_world = world::get(subworld_id);
		//		ImGui::Text(p_world->name.c_str());
		//	}
		// }

		// if (ImGui::CollapsingHeader("Entities"))
		//{
		//	for (auto& entity_id : p_world->entities)
		//	{
		//		char buffer[100] {};
		//		auto p_entity = entity::get(entity_id);
		//		std::copy_n(p_entity->name.begin(), p_entity->name.size(), buffer);
		//		if (ImGui::InputText(std::format("##{}", entity_id.ToString()).c_str(), buffer, 100, ImGuiInputTextFlags_AutoSelectAll))
		//		{
		//			p_entity->name = std::string(buffer);
		//		}
		//	}
		// }
	}

	void _draw_component(editor_id component_id)
	{
		// ImGui::Text(component::find(component_id)->name.c_str());
	}

	void _draw_entity(editor_id entity_id)
	{
		auto p_e = models::entity::find(entity_id);
		if (p_e is_nullptr) return;

		widgets::editable_header(entity_id, p_e->name);
		widgets::text(std::format("world : {}", models::world::find(p_e->world_id)->name).c_str());
		widgets::text(std::format("archetype : {}", p_e->archetype).c_str());
		widgets::text(std::format("ecs_idx : {}", p_e->ecs_idx).c_str());

		std::ranges::for_each(component::all(entity_id), [=](auto* p_c) {
			widgets::component_drag(p_c->id);
			// todo
			auto p_w = world::find(p_e->world_id);
			auto p_s = scene::find(p_w->scene_id);
			widgets::text(std::format("memory : {}", game::ecs::get_component_memory(p_s->ecs_idx, p_w->ecs_idx, p_e->ecs_idx, editor::models::reflection::find_struct(p_c->struct_id)->ecs_idx)).c_str());

			// when right click, allow various context munu popup?
			//	ex. struct => may show context menu (reflection) or may not show context menu (world archetype)
			//  ex. add struct / remove struct => contxt menu item whit checkbox
			//  ex. world => show different "add component"
			//	ex. entity in hiarachy vs in ecs debugger vs inspector
			//		each context (hiarachy -> entity vs ecs debugger -> entity vs inspector -> entity, maybe id stack) may have different context menu
			//
			//
			// rethink about editor select system
			// rethink about editor xml
			// context menu item size too big (world -> add struct/ remove struct)
		});

		// auto p_entity = entity::get(entity_id);

		// ImGui::Text((p_entity->name).c_str());

		// widgets::separator();

		// if (ImGui::CollapsingHeader("Components"))
		//{
		//	for (auto& component_id : p_entity->components)
		//	{
		//		_draw_component(component_id);
		//		// char buffer[100] {};
		//		// auto p_component = Component::Find(component_id);
		//		// std::copy_n(p_component->Name.begin(), p_component->Name.size(), buffer);
		//		// if (ImGui::InputText(std::format("##{}", component_id.ToString()).c_str(), buffer, 100, ImGuiInputTextFlags_AutoSelectAll))
		//		//{
		//		//	p_component->Name = std::string(buffer);
		//		// }
		//	}
		// }
	}

	void show()
	{
		if (_open is_false) return;
		if (widgets::begin("Inspector", &_open))
		{
			auto id = get_current_selection();
			switch (id.type())
			{
			case DataType_Scene:
				_draw_scene(id);
				break;
			case DataType_World:
				_draw_world(id);
				break;
			case DataType_Entity:
				_draw_entity(id);
				break;
			case DataType_Component:
				if (component::find(id))
				{
					_draw_entity(component::find(id)->entity_id);
				}
				break;
			default:
				break;
			}
		}

		widgets::end();
	}
}	 // namespace editor::view::inspector

namespace editor::view::reflection
{
	using namespace editor::models;

	auto _open = true;

	const auto cmd_open = editor_command {
		"Open reflection Window",
		ImGuiKey_None,
		[](editor_id _) {
			return true;
		},
		[](editor_id _) {
			_open ^= true;
		}
	};

	void show()
	{
		if (_open is_false) return;

		if (widgets::begin("Reflection", &_open))
		{
			for (auto* p_s : models::reflection::all_structs())
			{
				if (widgets::tree_node(p_s->name.c_str()))
				{
					for (auto* p_f : models::reflection::all_fields(p_s->id))
					{
						auto selected = false;
						widgets::tree_node(std::format("{} ({}) : ( {} )",
													   p_f->name,
													   models::reflection::utils::type_to_string(p_f->type),
													   models::reflection::utils::deserialize(p_f->type, game::ecs::get_field_pvalue(p_s->ecs_idx, p_f->ecs_idx))),
										   ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
					}
					widgets::tree_pop();
				}
			}
		}

		widgets::end();
	}
}	 // namespace editor::view::reflection

namespace editor::view::world_editor
{
	using namespace editor::models;

	auto _open = true;

	const auto cmd_open = editor_command {
		"Open reflection Window",
		ImGuiKey_None,
		[](editor_id _) {
			return true;
		},
		[](editor_id _) {
			_open ^= true;
		}
	};

	void show()
	{
		if (_open is_false) return;

		if (widgets::begin("World Editor", &_open))
		{

			widgets::text("Hi Im World Editor");
			// std::ranges::for_each(editor::models::reflection::all_structs(), [](em_struct* p_s) {
			//	if (widgets::tree_node(p_s->name.c_str()))
			//	{
			//		std::ranges::for_each(editor::models::reflection::all_fields(p_s->id), [](em_field* p_f) {
			//			auto selected = false;
			//			widgets::tree_node(std::format("{} ({}) : ( {} )", p_f->name, editor::models::reflection::utils::type_to_string(p_f->type), editor::models::reflection::utils::deserialize(p_f->type, p_f->p_value)), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			//		});


			//		widgets::tree_pop();
			//	}
			//});
		}

		widgets::end();
	}
}	 // namespace editor::view::world_editor

namespace editor::view::systems
{
	using namespace editor::models;

	auto _open = true;

	const auto cmd_open = editor_command {
		"Open systems Window",
		ImGuiKey_None,
		[](editor_id _) {
			return true;
		},
		[](editor_id _) {
			_open ^= true;
		}
	};

	void show()
	{
		if (_open is_false) return;

		if (widgets::begin("Systems", &_open) is_false)
		{
			widgets::end();
			return;
		}

		for (auto* p_sys : system::all())
		{
			if (widgets::tree_node(p_sys->name) is_false)
			{
				continue;
			}

			for (auto& [func_name, arg_type] : p_sys->interfaces)
			{
				auto arg_str = arg_type;
				if (func_name == "update")
				{
					arg_str = std::accumulate(
						p_sys->update_params.begin(),
						p_sys->update_params.end(),
						arg_type,
						[](std::string arg_str, editor_id struct_id) {
							return std::format("{}, {}", arg_str, editor::models::reflection::find_struct(struct_id)->name);
						});
				}

				widgets::tree_node(
					std::format("{}({})", func_name, arg_str),
					ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			}

			widgets::tree_pop();
		}

		widgets::end();
	}
}	 // namespace editor::view::systems

namespace editor::view
{
	namespace
	{
		void _main_dock_space()
		{
			auto   window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking;
			auto   mainViewPort = ImGui::GetMainViewport();
			auto   size			= mainViewPort->Size - ImVec2(0, CAPTION_HIGHT * GEctx->dpi_scale);
			ImVec2 pos			= mainViewPort->Pos + ImVec2(0, CAPTION_HIGHT * GEctx->dpi_scale);
			style::push_var(ImGuiStyleVar_WindowRounding, 0.0f);
			style::push_var(ImGuiStyleVar_WindowBorderSize, 0.0f);
			style::push_var(ImGuiStyleVar_WindowPadding, ImVec2());
			platform::set_next_window_pos(pos);
			platform::set_next_window_size(size);
			platform::set_next_window_viewport(mainViewPort->ID);
			if (widgets::begin("MainView", NULL, window_flags))
			{
				ImGui::DockSpace(ImGui::GetID("DockSpace"), size, ImGuiDockNodeFlags_None);	   // | ImGuiDockNodeFlags_NoSplit);
			}

			widgets::end();
			style::pop_var(3);
		}

		void _caption_bar()
		{
			auto viewport	  = platform::get_main_viewport();
			auto window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
			auto menuSize	  = ImVec2(viewport.size().x, CAPTION_HIGHT * GEctx->dpi_scale);

			style::push_color(ImGuiCol_WindowBg, style::get_color_v4(ImGuiCol_TitleBg));
			style::push_color(ImGuiCol_ChildBg, style::get_color_v4(ImGuiCol_TitleBg));
			style::push_var(ImGuiStyleVar_WindowBorderSize, 0.0f);
			style::push_var(ImGuiStyleVar_WindowPadding, ImVec2());

			platform::set_next_window_viewport(viewport.id());
			platform::set_next_window_pos(viewport.pos());
			platform::set_next_window_size(menuSize);
			if (widgets::begin("Caption", nullptr, window_flags))
			{
				editor::view::main_menu_bar::show();
#pragma region caption buttons
				auto  dpi_scale					  = GEctx->dpi_scale;
				auto& style						  = GImGui->Style;
				auto  caption_button_icon_width	  = 5.f * dpi_scale;
				auto  caption_restore_icon_offset = 2.f * dpi_scale;
				auto  caption_button_size		  = ImVec2(CAPTION_BUTTON_SIZE.x * dpi_scale, CAPTION_BUTTON_SIZE.y * dpi_scale);
				auto  draw_pos					  = ImVec2(platform::get_window_pos().x + platform::get_window_size().x - caption_button_size.x * 3, platform::get_window_pos().y);
				auto  caption_button_icon_center  = ImVec2(draw_pos.x + caption_button_size.x / 2, draw_pos.y + caption_button_size.y / 2);

				if (GEctx->caption_hovered_btn != Caption_Button_None)
				{
					auto p_min = ImVec2(draw_pos.x + CAPTION_BUTTON_SIZE.x * (int)GEctx->caption_hovered_btn * GEctx->dpi_scale, draw_pos.y);
					auto p_max = ImVec2(p_min.x + CAPTION_BUTTON_SIZE.x * GEctx->dpi_scale, draw_pos.y + CAPTION_BUTTON_SIZE.y * GEctx->dpi_scale);
					widgets::draw_rect_filled({ p_min, p_max }, style::get_color_u32(GEctx->caption_held_btn == GEctx->caption_hovered_btn ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered));
				}

				widgets::draw_line(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y), ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y), style::get_color_u32(ImGuiCol_Text), dpi_scale);

				draw_pos.x					 += caption_button_size.x;
				caption_button_icon_center.x += caption_button_size.x;

				if (platform::is_window_maximized(GEctx->hwnd))
				{
					widgets::draw_rect(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width + caption_restore_icon_offset),
									   ImVec2(caption_button_icon_center.x + caption_button_icon_width - caption_restore_icon_offset, caption_button_icon_center.y + caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), 0.f, ImDrawFlags_None, dpi_scale);

					widgets::draw_line(ImVec2(caption_button_icon_center.x - caption_button_icon_width + caption_restore_icon_offset, caption_button_icon_center.y - caption_button_icon_width + caption_restore_icon_offset),
									   ImVec2(caption_button_icon_center.x - caption_button_icon_width + caption_restore_icon_offset, caption_button_icon_center.y - caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), dpi_scale);

					widgets::draw_line(ImVec2(caption_button_icon_center.x - caption_button_icon_width + caption_restore_icon_offset, caption_button_icon_center.y - caption_button_icon_width),
									   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), dpi_scale);

					widgets::draw_line(ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width),
									   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width - caption_restore_icon_offset), style::get_color_u32(ImGuiCol_Text), dpi_scale);

					widgets::draw_line(ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width - caption_restore_icon_offset),
									   ImVec2(caption_button_icon_center.x + caption_button_icon_width - caption_restore_icon_offset, caption_button_icon_center.y + caption_button_icon_width - caption_restore_icon_offset), style::get_color_u32(ImGuiCol_Text), dpi_scale);
				}
				else
				{
					widgets::draw_rect(ImVec2(draw_pos.x + caption_button_size.x / 2 - caption_button_icon_width, draw_pos.y + caption_button_size.y / 2 - caption_button_icon_width), ImVec2(draw_pos.x + caption_button_size.x / 2 + caption_button_icon_width, draw_pos.y + caption_button_size.y / 2 + caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), 0.f, ImDrawFlags_None, dpi_scale);
				}

				draw_pos.x					 += caption_button_size.x;
				caption_button_icon_center.x += caption_button_size.x;

				widgets::draw_line(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width),
								   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), dpi_scale);
				widgets::draw_line(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width),
								   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), dpi_scale);
#pragma endregion
			}

			style::pop_var(2);
			style::pop_color(2);

			widgets::end();
		}
	}	 // namespace

	void on_project_loaded()
	{
		auto res  = true;
		res		 &= editor::ctx_item::add_context_item("Main Menu\\Window\\Hierarchy", &hierarchy::cmd_open);
		res		 &= editor::ctx_item::add_context_item("Main Menu\\Window\\Inspector", &inspector::cmd_open);
		res		 &= editor::ctx_item::add_context_item("Main Menu\\Window\\Reflection", &reflection::cmd_open);
		res		 &= editor::ctx_item::add_context_item("Main Menu\\Window\\World_Editor", &editor::view::world_editor::cmd_open);
		res		 &= editor::ctx_item::add_context_item("Main Menu\\Window\\Systems", &editor::view::systems::cmd_open);
		assert(res);
		logger::on_project_loaded();
	}

	void update_dpi_scale()
	{
		project_browser::update_dpi_scale();
	}

	void init()
	{
	}

	void show()
	{
		_caption_bar();
		_main_dock_space();
		logger::show();

		if (editor::game::get_pproject()->is_opened is_false)
		{
			project_browser::show();
		}
		else if (editor::game::get_pproject()->is_loaded)
		{
			hierarchy::show();
			inspector::show();
			reflection::show();
			world_editor::show();
			systems::show();
			ctx_popup::show();
		}

		ImGui::ShowDemoWindow();
	}

	void on_project_unloaded()
	{
	}
}	 // namespace editor::view
