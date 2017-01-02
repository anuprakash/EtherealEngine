#include "model.h"
#include "material.h"
#include "mesh.h"
#include "core/math/math_includes.h"
#include "../assets/asset_manager.h"


Model::Model()
{
	auto am = core::get_subsystem<runtime::AssetManager>();
	am->load<Material>("engine_data://materials/standard", false)
		.then([this](auto asset)
	{
		_materials.push_back(asset);
	});
}

bool Model::is_valid() const
{
	return !_mesh_lods.empty();
}

AssetHandle<Mesh> Model::get_lod(std::uint32_t lod) const
{
	if (_mesh_lods.size() > lod)
	{
		auto lodMesh = _mesh_lods[lod];
		if (lodMesh)
			return lodMesh;

		for (unsigned int i = lod; i < _mesh_lods.size(); ++i)
		{
			auto lodMesh = _mesh_lods[i];
			if (lodMesh)
				return lodMesh;
		}
		for (unsigned int i = lod; i > 0; --i)
		{
			auto lodMesh = _mesh_lods[i];
			if (lodMesh)
				return lodMesh;
		}
	}
	return AssetHandle<Mesh>();
}

void Model::set_lod(AssetHandle<Mesh> mesh, std::uint32_t lod)
{
	if (lod >= _mesh_lods.size())
		_mesh_lods.resize(lod + 1);

	_mesh_lods[lod] = mesh;
}

void Model::set_material(AssetHandle<Material> material, std::uint32_t index)
{
	if (index >= _mesh_lods.size())
		_mesh_lods.resize(index + 1);

	_materials[index] = material;
}

const std::vector<AssetHandle<Mesh>>& Model::get_lods() const
{
	return _mesh_lods;
}

void Model::set_lods(const std::vector<AssetHandle<Mesh>>& lods)
{
	_mesh_lods = lods;
}


const std::vector<AssetHandle<Material>>& Model::get_materials() const
{
	return _materials;
}

void Model::set_materials(const std::vector<AssetHandle<Material>>& materials)
{
	_materials = materials;
}


AssetHandle<Material> Model::get_material_for_group(const Group& group) const
{
	// TODO implement this
	if (_materials.empty())
		return AssetHandle<Material>();

	return _materials[0];
}

void Model::set_lod_max_distance(float distance)
{
	if (distance < _min_distance)
		distance = _min_distance;

	_max_distance = distance;
}

void Model::set_lod_min_distance(float distance)
{
	if (distance > _max_distance)
		distance = _max_distance;

	_min_distance = distance;
}