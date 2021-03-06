#include "vertex_buffer.h"

VertexBuffer::~VertexBuffer()
{
	dispose();
}

void VertexBuffer::dispose()
{
	if (is_valid())
		gfx::destroyVertexBuffer(handle);

	handle = { bgfx::invalidHandle };
}

bool VertexBuffer::is_valid() const
{
	return gfx::isValid(handle);
}

void VertexBuffer::populate(const gfx::Memory* _mem, const gfx::VertexDecl& _decl, std::uint16_t _flags /*= BGFX_BUFFER_NONE*/)
{
	dispose();

	handle = gfx::createVertexBuffer(_mem, _decl, _flags);
}
