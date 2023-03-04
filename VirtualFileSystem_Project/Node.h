#pragma once
#include <map>
#include <string>

class Node
{
private:
	std::map<std::string, std::pair<uint32_t, Node*>> _children;
public:
	void Add(std::string_view name, uint32_t addr);
	void Destroy();
	uint32_t GetAddr(std::string_view name);
	void BindNewTreeToChild(const std::string& name, Node* nodePtr);
	void Print(uint32_t count = 0);

};

