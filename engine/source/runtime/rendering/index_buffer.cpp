#include "index_buffer.h"

IndexBuffer::~IndexBuffer()
{
	dispose();
}

void IndexBuffer::dispose()
{
	if (is_valid())
		gfx::destroyIndexBuffer(handle);

	handle = { bgfx::invalidHandle };
}

bool IndexBuffer::is_valid() const
{
	return gfx::isValid(handle);
}

void IndexBuffer::populate(const gfx::Memory* _mem, std::uint16_t _flags /*= BGFX_BUFFER_NONE*/)
{
	dispose();
	handle = gfx::createIndexBuffer(_mem, _flags);
}
