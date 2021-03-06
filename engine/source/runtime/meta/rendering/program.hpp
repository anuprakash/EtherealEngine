#pragma once
#include "core/serialization/serialization.h"
#include "core/serialization/cereal/types/vector.hpp"
#include "core/reflection/reflection.h"
#include "core/logging/logging.h"
#include "../../rendering/program.h"
#include "../assets/asset_handle.hpp"

SAVE(Program)
{
	try_save(ar, cereal::make_nvp("shaders", shaders));
}

LOAD(Program)
{
	std::vector<AssetHandle<Shader>> shaders;

	try_load(ar, cereal::make_nvp("shaders", shaders));

	for (auto shader : shaders)
	{
		obj.add_shader(shader);
	}
	obj.populate();
}
