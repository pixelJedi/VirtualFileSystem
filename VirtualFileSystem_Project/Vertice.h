#pragma once
#include <map>
#include <string>
#include <sstream>
#include <memory>
#include "IVFS.h"

template <typename T>
class Vertice
{
private:
	std::map<std::string, std::pair<std::unique_ptr<T>, Vertice*>> _children;
public:
	inline static char DELIMITER = '\\';

	void Add(std::string_view path, const T& data);	// Creates all the vertices within path, with data generated on the flow
	void Destroy();									// Recursively deletes all the children tree
	void BindNewTreeToChild(const std::string& name, Vertice* nodePtr, bool deleteData = false);

	T GetData(std::string_view path);
	uint32_t Count();

	std::string PrintVerticeTree(uint32_t count = 0);
	friend void TreeToPlain(std::vector<char*>& info, Vertice<T>& tree, uint32_t& nodecode);
};

template <typename T> void Vertice<T>::Add(std::string_view path, const T& data)
{
	size_t pos = path.find_first_of(DELIMITER);
	std::string head = std::string{ path.substr(0, pos) };	// Parse current node name
	if (!_children.count(head))
		_children[head] = std::pair<std::unique_ptr<T>, Vertice*>(nullptr, nullptr);
	if (pos < path.length())	// == not a leaf
	{
		if (_children[head].first) throw std::invalid_argument(" Cannot attach to a leaf: " + head);
		if (!_children[head].second) _children[head].second = new Vertice();
		_children[head].second->Add(path.substr(pos + 1, path.length() - (pos + 1)), data);
	}
	else
	{
		_children[head].first = std::make_unique<T>(data);
	}
}

template <typename T> void Vertice<T>::Destroy()
{
	if (_children.size())
	{
		for (auto iter = _children.begin(); iter != _children.end(); ++iter)
		{ 
			if ((*iter).second.second)
			{
				((*iter).second.second)->Destroy();
				delete (*iter).second.second;
			}
		}
		_children.clear();
	}
}

template <typename T> T Vertice<T>::GetData(std::string_view path)
{
	size_t pos = path.find_first_of(DELIMITER);
	std::string head = std::string{ path.substr(0, pos) };
	if (!_children.count(head)) throw std::logic_error(head + " does not exist");
	
	if (pos < path.length())	// == not a leaf
	{
		if (!_children[head].second) throw std::logic_error(head + " is not a directory");
		return _children[head].second->GetData(path.substr(pos + 1, path.length() - (pos + 1)));
	}
	else
		return *_children[head].first;
}

template <typename T> uint32_t Vertice<T>::Count()
{
	uint32_t count = _children.size();
	for (auto iter = _children.begin(); iter != _children.end(); ++iter)
	{
		count += (*iter).second.second->Count();
	}
	return count;
}

template <typename T> void Vertice<T>::BindNewTreeToChild(const std::string& name, Vertice<T>* nodePtr, bool deleteData)
{
	if (deleteData) _children[name].second->Destroy();
	_children[name].second = nodePtr;
}

template <typename T> std::string Vertice<T>::PrintVerticeTree(uint32_t count)
{
	std::ostringstream info;
	if (!_children.empty())
	{
		for (auto iter = _children.begin(); iter != _children.end(); ++iter)
		{
			for (uint32_t i = 0; i != count; ++i) info << "  ";
			info << (*iter).first << " ";
			if ((*iter).second.first) info << *((*iter).second.first);
			else info << "-";
			info << "\n";
			if ((*iter).second.second) info << (*iter).second.second->PrintVerticeTree(count + 1);
		}
	}
	return info.str();
}
