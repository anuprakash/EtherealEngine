#pragma once

#include "uniform.h"
#include <memory>
#include <vector>

struct Shader
{
	//-----------------------------------------------------------------------------
	//  Name : ~Shader ()
	/// <summary>
	/// 
	/// 
	/// 
	/// </summary>
	//-----------------------------------------------------------------------------
	~Shader();

	//-----------------------------------------------------------------------------
	//  Name : dispose ()
	/// <summary>
	/// 
	/// 
	/// 
	/// </summary>
	//-----------------------------------------------------------------------------
	void dispose();

	//-----------------------------------------------------------------------------
	//  Name : is_valid ()
	/// <summary>
	/// 
	/// 
	/// 
	/// </summary>
	//-----------------------------------------------------------------------------
	bool is_valid() const;

	//-----------------------------------------------------------------------------
	//  Name : populate ()
	/// <summary>
	/// 
	/// 
	/// 
	/// </summary>
	//-----------------------------------------------------------------------------
	void populate(const gfx::Memory* _mem);

	/// Uniforms for this shader
	std::vector<std::shared_ptr<Uniform>> uniforms;
	/// Internal handle
	gfx::ShaderHandle handle = { gfx::invalidHandle };
};