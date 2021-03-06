/*
 * Copyright 2011-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "shaderc.h"

namespace bgfx
{
	bool compilePSSLShader(bx::CommandLine& _cmdLine, uint32_t _version, const std::string& _code, bx::WriterI* _writer, std::string& err)
	{
		BX_UNUSED(_cmdLine, _version, _code, _writer);
		bx::stringPrintf(err, "PSSL compiler is not supported.\n");
		fprintf(stderr, "PSSL compiler is not supported.\n");
		return false;
	}

} // namespace bgfx
