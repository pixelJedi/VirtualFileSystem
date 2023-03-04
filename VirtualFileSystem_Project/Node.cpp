#include <iostream>
#include "Node.h"

void Node::Add(std::string_view name, uint32_t addr)
{
	// todo: change addr to a delegate that calculates correct addr from the external bitmap
	// note: currently all addresses are the same
	size_t pos = name.find_first_of("\\");
	std::string head = std::string{ name.substr(0, pos) };
	if (!_children.count(head)) _children[head] = std::pair<uint32_t, Node*>(addr, new Node());
	if (pos < name.length())
	{
		_children[head].second->Add(name.substr(pos + 1, name.length() - (pos + 1)), addr);
	}
}
void Node::Destroy()
{
	if (_children.size())
	{
		for (auto iter = _children.begin(); iter != _children.end(); ++iter)
		{
			((* iter).second.second) -> Destroy();
			delete iter->second.second;
		}
		_children.clear();
	}
}

uint32_t Node::GetAddr(std::string_view name)
{
	size_t pos = name.find_first_of("\\");
	std::string head = std::string{ name.substr(0, pos) };
	if (pos < name.length())
	{
		if (!_children.count(head)) 
			throw std::logic_error("File or directory does not exist: " + head);
		else return GetAddr(name.substr(pos+1, name.length() - (pos + 1)));
	}
	else
	{
		return _children[head].first;
	}
}

void Node::Print(uint32_t count)
{
	if (!_children.empty())
	{
		for (auto iter = _children.begin(); iter != _children.end(); ++iter)
		{
			for (uint32_t i = 0; i != count; ++i) std::cout << "  ";
			std::cout << (*iter).first << "\n";
			if (!((*iter).second.second->_children.empty())) (*iter).second.second->Print(count+1);
		}
	}
}
