#pragma once
#include <map>
#include <string>

template <typename T>
class Vertice
{
private:
	std::map<std::string, std::pair<T*, Vertice*>> _children;
public:
	inline static char DELIMITER = '\\';

	void Add(std::string_view path, T* data);	// Creates all the vertices within path, with data generated on the flow
	void Destroy(bool deleteData = true);		// Recursively deletes all the children tree
	void BindNewTreeToChild(const std::string& name, Vertice* nodePtr, bool deleteData = true);

	T* GetData(std::string_view path);
	uint32_t Count();
};


template <typename T>
void Vertice<T>::Add(std::string_view path, T* data)
{
	size_t pos = path.find_first_of(DELIMITER);
	std::string head = std::string{ path.substr(0, pos) };
	if (!_children.count(head)) _children[head] = std::pair<T*, Vertice*>(data, nullptr);
	if (pos < path.length())	// == not a leaf
	{
		_children[head].second = new Vertice();
		_children[head].second->Add(path.substr(pos + 1, path.length() - (pos + 1)), data);
	}
}

template <typename T>
void Vertice<T>::Destroy(bool deleteData)
{
	if (_children.size())
	{
		for (auto iter = _children.begin(); iter != _children.end(); ++iter)
		{
			((*iter).second.second)->Destroy();
			if (deleteData) delete iter->second.first;
			delete iter->second.second;
		}
		_children.clear();
	}
}

template <typename T>
T* Vertice<T>::GetData(std::string_view path)
{
	size_t pos = path.find_first_of(DELIMITER);
	std::string head = std::string{ path.substr(0, pos) };
	if (!_children.count(head)) throw std::logic_error("Directory does not exist: " + head);
	if (!_children[head].second) throw std::logic_error(head + " is not a directory");
	if (pos < path.length())	// == not a leaf
		return _children[head].second->GetData(path.substr(pos + 1, path.length() - (pos + 1)));
	else
		return _children[head].first;
}

template <typename T>
uint32_t Vertice<T>::Count()
{
	uint32_t count = _children.size();
	for (auto iter = _children.begin(); iter != _children.end(); ++iter)
	{
		count += (*iter).second.second->Count();
	}
	return count;
}

template <typename T>
void Vertice<T>::BindNewTreeToChild(const std::string& name, Vertice* nodePtr, bool deleteData)
{
	if (deleteData) _children[name].second->Destroy();
	_children[name].second = nodePtr;
}