#include "docks.h"
#include "../../edit_state.h"
#include "runtime/ecs/prefab.h"
#include "runtime/ecs/utils.h"
#include "runtime/ecs/components/transform_component.h"
#include "runtime/ecs/components/model_component.h"
#include "runtime/ecs/systems/transform_system.h"
#include "runtime/input/input.h"
#include "runtime/system/filesystem.h"
#include "runtime/rendering/mesh.h"
namespace Docks
{

	void check_context_menu(runtime::Entity entity)
	{
		auto es = core::get_subsystem<editor::EditState>();
		auto ecs = core::get_subsystem<runtime::EntityComponentSystem>();
		auto& editor_camera = es->camera;
		if (entity && entity != editor_camera)
		{
			if (gui::BeginPopupContextItem("Entity Context Menu"))
			{
				if (gui::Selectable("Create child"))
				{
					auto object = ecs->create();
					object.assign<TransformComponent>().lock()
						->set_parent(entity.component<TransformComponent>());
				}
				if (gui::Selectable("Clone"))
				{
					auto object = ecs->create_from_copy(entity);
					object.component<TransformComponent>().lock()
						->set_parent(entity.component<TransformComponent>().lock()->get_parent(), false, true);
					
					es->select(object);
				}
				if (gui::Selectable("Save As Prefab"))
				{
					ecs::utils::save_entity("data://prefabs/", entity);
				}
				if (gui::Selectable("Delete"))
				{
					entity.destroy();
				}

				gui::EndPopup();
			}
		}
		else
		{
			if (gui::BeginPopupContextWindow())
			{
				if (gui::Selectable("Create empty"))
				{
					auto object = ecs->create();
					object.assign<TransformComponent>();
				}

				gui::EndPopup();
			}
		}


	}

	void check_drag(runtime::Entity entity)
	{
		if (!gui::IsWindowHovered())
			return;

		auto es = core::get_subsystem<editor::EditState>();
		auto& editor_camera = es->camera;
		auto& dragged = es->drag_data.object;

		auto ecs = core::get_subsystem<runtime::EntityComponentSystem>();

		if (entity)
		{
			if (gui::IsItemHoveredRect())
			{
				if (gui::IsMouseClicked(gui::drag_button) &&
					entity != editor_camera)
				{
					es->drag(entity, entity.to_string());
				}
				if (gui::IsMouseReleased(gui::drag_button))
				{
					if (dragged && entity != editor_camera)
					{
						if (dragged.is_type<runtime::Entity>())
						{
							
							auto dragged_entity = dragged.get_value<runtime::Entity>();
							dragged_entity.component<TransformComponent>().lock()
								->set_parent(entity.component<TransformComponent>());

							es->drop();
						}

						if (dragged.is_type<AssetHandle<Prefab>>())
						{
							auto prefab = dragged.get_value<AssetHandle<Prefab>>();
							auto object = prefab->instantiate();
							object.component<TransformComponent>().lock()
								->set_parent(entity.component<TransformComponent>());

							es->drop();
							es->select(object);
						}
						if (dragged.is_type<AssetHandle<Mesh>>())
						{
							auto mesh = dragged.get_value<AssetHandle<Mesh>>();
							Model model;		
							model.set_lod(mesh, 0);

							auto object = ecs->create();
							//Add component and configure it.
							object.assign<TransformComponent>().lock()
								->set_parent(entity.component<TransformComponent>());
							//Add component and configure it.
							object.assign<ModelComponent>().lock()
								->set_casts_shadow(true)
								.set_casts_reflection(false)
								.set_model(model);

							es->drop();
							es->select(object);
						}
					}
				}
			}
		}
		
	}

	bool is_parent_of(TransformComponent* trans, TransformComponent* selected_ransform)
	{
		if (!selected_ransform)
			return false;

		if (trans == selected_ransform)
		{
			return true;
		}
		auto parent = selected_ransform->get_parent().lock().get();
		return is_parent_of(trans, parent);
	};

	void draw_entity(runtime::Entity entity)
	{
		if (!entity)
			return;

		gui::PushID(entity.id().index());
		gui::AlignFirstTextHeightToWidgets();
		auto es = core::get_subsystem<editor::EditState>();
		auto& selected = es->selection_data.object;

		bool is_selected = false;
		if (selected && selected.is_type<runtime::Entity>())
		{
			is_selected = selected.get_value<runtime::Entity>() == entity;
		}	
		
		bool is_parent_of_selected = false;
// 		if (selected && selected.is_type<runtime::Entity>())
// 		{
// 			auto selectedEntity = selected.get_value<runtime::Entity>();
// 			auto selectedTransformComponent = selectedEntity.component<TransformComponent>().lock()->getParent().lock().get();
// 			auto entityTransformComponent = entity.component<TransformComponent>().lock().get();
// 			isParentOfSelected = isParentOf(entityTransformComponent, selectedTransformComponent);
// 		}	
		std::string name = entity.to_string();
		ImGuiTreeNodeFlags flags = 0
			| ImGuiTreeNodeFlags_OpenOnDoubleClick
			| ImGuiTreeNodeFlags_AllowOverlapMode
			| ImGuiTreeNodeFlags_OpenOnArrow;

		if (is_selected)
			flags |= ImGuiTreeNodeFlags_Selected;


		auto transformComponent = entity.component<TransformComponent>().lock();
		bool no_children = true;
		if (transformComponent)
			no_children = transformComponent->get_children().empty();

		if (no_children)
			flags |= ImGuiTreeNodeFlags_Leaf;

		if(is_parent_of_selected)
			gui::SetNextTreeNodeOpen(true);

		bool opened = gui::TreeNodeEx(name.c_str(), flags);
		if (gui::IsItemClicked(0))
		{
			es->select(entity);
		}

		check_context_menu(entity);
		check_drag(entity);
		if (opened)
		{
			if (!no_children)
			{
				auto children = entity.component<TransformComponent>().lock()->get_children();
				for (auto& child : children)
				{
					if (!child.expired())
						draw_entity(child.lock()->get_entity());
				}
			}

			gui::TreePop();
		}

		gui::PopID();
	}

	void render_hierarchy(ImVec2 area)
	{
		auto es = core::get_subsystem<editor::EditState>();
		auto ecs = core::get_subsystem<runtime::EntityComponentSystem>();
		auto ts = core::get_subsystem<runtime::TransformSystem>();
		auto input = core::get_subsystem<runtime::Input>();

		auto& roots = ts->get_roots();

		auto& editor_camera = es->camera;
		auto& selected = es->selection_data.object;
		auto& dragged = es->drag_data.object;

		check_context_menu(runtime::Entity());

		if (gui::IsWindowFocused())
		{
			if (input->is_key_pressed(sf::Keyboard::Delete))
			{
				if (selected && selected.is_type<runtime::Entity>())
				{
					auto sel = selected.get_value<runtime::Entity>();
					if (sel != editor_camera)
					{
						sel.destroy();
						es->unselect();
					}
				}
			}

			if (input->is_key_pressed(sf::Keyboard::D))
			{
				if (input->is_key_down(sf::Keyboard::LControl))
				{
					if (selected && selected.is_type<runtime::Entity>())
					{
						auto sel = selected.get_value<runtime::Entity>();
						if (sel != editor_camera)
						{
							auto clone = ecs->create_from_copy(sel);
							clone.component<TransformComponent>().lock()
								->set_parent(sel.component<TransformComponent>().lock()->get_parent(), false, true);
							es->select(clone);
						}
					}
				}
			}
		}

		for (auto& root : roots)
		{
			if (!root.expired())
				draw_entity(root.lock()->get_entity());
		}	

		if (gui::IsWindowHovered() && !gui::IsAnyItemHovered())
		{
			if (gui::IsMouseReleased(gui::drag_button))
			{
				if (dragged)
				{
					if (dragged.is_type<runtime::Entity>())
					{
						auto dragged_entity = dragged.get_value<runtime::Entity>();
						dragged_entity.component<TransformComponent>().lock()
							->set_parent(runtime::CHandle<TransformComponent>());

						es->drop();
					}
					if (dragged.is_type<AssetHandle<Prefab>>())
					{
						auto prefab = dragged.get_value<AssetHandle<Prefab>>();
						auto object = prefab->instantiate();
						es->drop();
						es->select(object);

					}
					if (dragged.is_type<AssetHandle<Mesh>>())
					{
						auto mesh = dragged.get_value<AssetHandle<Mesh>>();
						Model model;
						model.set_lod(mesh, 0);

						auto object = ecs->create();
						//Add component and configure it.
						object.assign<TransformComponent>();
						//Add component and configure it.
						object.assign<ModelComponent>().lock()
							->set_casts_shadow(true)
							.set_casts_reflection(false)
							.set_model(model);

						es->drop();
						es->select(object);
					}
				}
			}
		}
	}

};