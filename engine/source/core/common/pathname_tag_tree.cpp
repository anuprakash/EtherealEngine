#include "pathname_tag_tree.h"
#include <deque>

PathNameTagTree::node::node() :
	_nodes(),
	_tag(-1)
{ }
PathNameTagTree::node::node(const node& krs) :
	_nodes(krs._nodes),
	_tag(krs._tag)
{ }

PathNameTagTree::PathNameTagTree(const char sep_char, const size_t invalid_tag) :
	_invalid_tag(invalid_tag),
	_sep_char(sep_char)
{
	_root_node._tag = _invalid_tag;
}

void PathNameTagTree::reset()
{
	_root_node._nodes.clear();
	_root_node._tag = _invalid_tag;
}

bool PathNameTagTree::set(const char* pathname)
{
	return set(pathname, _invalid_tag);
}

bool PathNameTagTree::set(size_t tag)
{
	_root_node._tag = tag;

	return tag != _invalid_tag;
}

bool PathNameTagTree::get(size_t& tag)
{
	tag = _root_node._tag;

	return tag != _invalid_tag;
}

bool PathNameTagTree::set(std::string pathname, size_t tag)
{
	size_t i = pathname.find_first_of(_sep_char, 0);

	node* n = &_root_node;
	
	std::deque<node*> back_trace;

	if (i == std::string::npos)
	{
		const char* name = pathname.c_str();

		if (n->_nodes.empty())
		{
			n->_tag = 0;
		}

		n = &n->_nodes[name];
	}
	else
	{
		for (std::string::iterator it = pathname.begin(); it != pathname.end();)
		{
			const char* name = get_name(pathname, it, i);

			if (n->_nodes.empty())
			{
				n->_tag = 0;
			}

			back_trace.push_back(n);

			n = &n->_nodes[name];
		}
	}

	

	bool invalid_tag = tag == _invalid_tag;

	if (i != std::string::npos)
	{
		tag = _invalid_tag;
	}

	if (n->_tag != tag)
	{
		n->_tag = tag;

		if (invalid_tag)
		{
			for (auto it : back_trace)
			{
				if (it->_tag > 0)
				{
					--it->_tag;
				}
			}
		}
		else
		{
			for (auto it : back_trace)
			{
				++it->_tag;
			}
		}
	}

	return true;
}

PathNameTagTree::iterator PathNameTagTree::create_iterator(std::string pathname)
{
	return iterator(&get_node_ref(pathname));
}

void PathNameTagTree::setup_iterator(iterator& it, std::string pathname)
{
	it.setup(&get_node_ref(pathname));
}

bool PathNameTagTree::get(std::string pathname, size_t& tag)
{
	node* n = get_node_ptr(pathname);

	tag = n == nullptr ? _invalid_tag : n->_tag;

	return tag != _invalid_tag;
}

const char* PathNameTagTree::get_name(std::string& pathname, std::string::iterator& it, size_t& i)
{
	size_t o = it - pathname.begin();

	if (o > 0)
	{
		i = pathname.find_first_of(_sep_char, o);
	}
	else
	{
		pathname[i] = 0;
	}

	if (i != std::string::npos)
	{
		it = pathname.begin() + i + 1;

		pathname[i] = 0;
	}
	else
	{
		it = pathname.end();
	}

	return &pathname[o];
}

PathNameTagTree::node* PathNameTagTree::get_node_ptr(std::string& pathname)
{
	bool b;

	node* n = get_node(pathname, b);

	return b ? n : nullptr;

	return nullptr;
}

PathNameTagTree::node& PathNameTagTree::get_node_ref(std::string& pathname)
{
	bool b;

	node* n = get_node(pathname, b);

	return n ? *n : _root_node;
}

PathNameTagTree::node* PathNameTagTree::get_node(std::string& pathname, bool& endpoint)
{
	endpoint = false;

	node* n = nullptr;

	size_t i = pathname.find_first_of(_sep_char, 0);

	if (i != std::string::npos)
	{
		n = &_root_node;

		for (std::string::iterator it = pathname.begin(); it != pathname.end();)
		{
			const char* name = get_name(pathname, it, i);

			auto it_node = n->_nodes.find(name);

			if (it_node == n->_nodes.end())
			{
				return nullptr;
			}

			n = &it_node->second;

			if (i == std::string::npos)
			{
				endpoint = true;

				break;
			}
		}
	}

	return n;
}

PathNameTagTree::iterator::iterator() { }
PathNameTagTree::iterator::iterator(void* p)
{
	setup(p);
}
PathNameTagTree::iterator::iterator(const iterator& krc) :
	_steps(krc._steps)
{ }

void PathNameTagTree::iterator::setup(void* p)
{
	do_step((node*)p);
}

bool PathNameTagTree::iterator::step_stack_push()
{
	if (_steps.empty())
	{
		return false;
	}

	_stack.push(_steps.back());

	return true;
}

bool PathNameTagTree::iterator::step_stack_pop(bool replace)
{
	if (_stack.empty())
	{
		return false;
	}

	if (replace)
	{
		_steps.back() = _stack.top();
	}
	else
	{
		_steps.push_back(_stack.top());
	}

	_stack.pop();
	
	return true;
}

bool PathNameTagTree::iterator::step()
{
	auto& it = _steps.back();

	++it.first;

	return it.first != it.second->_nodes.end();
}
bool PathNameTagTree::iterator::step_in()
{
	return do_step(&_steps.back().first->second);
}

bool PathNameTagTree::iterator::step_out()
{
	if (_steps.empty())
	{
		return false;
	}

	_steps.pop_back();

	return _steps.size() > 0;
}
bool PathNameTagTree::iterator::step_reset() 
{
	if (_steps.empty())
	{
		return false;
	}

	auto& it = _steps.back();

	it.first = it.second->_nodes.begin();

	return true;
}

bool PathNameTagTree::iterator::steping()
{
	auto& it = _steps.back();

	return it.first != it.second->_nodes.end();
}
size_t PathNameTagTree::iterator::steps()
{
	return _steps.size();
}
bool PathNameTagTree::iterator::do_step(node* n)
{
	if (n->_nodes.empty())
	{
		return false;
	}

	_steps.push_back(step_t(n->_nodes.begin(), n));

	return true;
}


const bool PathNameTagTree::iterator::is_leaf()
{
	return _steps.back().first->second._nodes.empty();
}
const std::string& PathNameTagTree::iterator::name()
{
	return _steps.back().first->first;
}
const size_t& PathNameTagTree::iterator::tag()
{
	return _steps.back().first->second._tag;
}
