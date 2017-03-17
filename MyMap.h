#ifndef MY_MAP
#define MY_MAP

#include <string>
#include "provided.h"

template<typename KeyType, typename ValueType>
class MyMap
{
public:
	MyMap() : m_rootPtr(nullptr), m_size(0) {}
	~MyMap() { deleteAll(); }
	void clear() { deleteAll(); }
	int size() const { return m_size; }
	void associate(const KeyType& key, const ValueType& value);

	// for a map that can't be modified, return a pointer to const ValueType
	const ValueType* find(const KeyType& key) const;

	// for a modifiable map, return a pointer to modifiable ValueType
	ValueType* find(const KeyType& key)
	{
		return const_cast<ValueType*>(const_cast<const MyMap*>(this)->find(key));
	}

	// C++11 syntax for preventing copying and assignment
	MyMap(const MyMap&) = delete;
	MyMap& operator=(const MyMap&) = delete;

private:
	struct Node {
		KeyType m_key;
		ValueType m_value;
		Node *left, *right;
	};
	Node* m_rootPtr;
	int m_size; // need to make sure that this gets adjusted properly for insertion and deletion

	// private associate method that does the work of creating the new node and properly initializing
	// it. sets the pointer reference to the address of the node that's created
	void associate(const KeyType& key, const ValueType& value, Node* &pointerToModify);
	void deleteAll();
	void deleteThis(Node* toBeDeleted); // called by deleteAll at first and then calls itself recursively
};

template<typename KeyType, typename ValueType>
void MyMap<KeyType, ValueType>::associate(const KeyType& key, const ValueType& value)
{
	if (m_rootPtr == nullptr)
	{
		associate(key, value, m_rootPtr); // add new node for root pointer to point to
	}
	else
	{
		Node* current = m_rootPtr;
		while (true)
		{
			if (current->m_key < key) // need to ensure that keys have the appropriate comparison operators defined
			{
				if (current->right == nullptr)
				{
					associate(key, value, current->right);
					return;
				}
				else
					current = current->right;
			}
			else if (key < current->m_key) // current node's key is greater than the key argument that was passed in
			{
				if (current->left == nullptr)
				{
					associate(key, value, current->left);
					return;
				}
				else
					current = current->left;
			}
			else // so current->m_key == key
			{
				current->m_value = value; // just replace the value of the already existent node
				return;
			}
		}
	}
}

// called by public associate method. adds a leaf and links to "pointerToModify".
template<typename KeyType, typename ValueType>
void MyMap<KeyType, ValueType>::associate(const KeyType& key, const ValueType& value, Node* &pointerToModify)
{
	pointerToModify = new Node; // add the link step here
	pointerToModify->left = pointerToModify->right = nullptr; // set each child as nullptr when node is first created
	pointerToModify->m_key = key;
	pointerToModify->m_value = value;
	m_size++;
}

// non-recursive find. returns nullptr if not found and pointer to node if found
template<typename KeyType, typename ValueType>
const ValueType* MyMap<KeyType, ValueType>::find(const KeyType& key) const
{
	Node* current = m_rootPtr;
	while (current != nullptr)
	{
		if (current->m_key < key)
			current = current->right;
		else if (key < current->m_key)// current node's key is greater than key argument
			current = current->left;
		else // key == current->m_key
			return &current->m_value;
	}

	return nullptr; // if find proceeds until it follows a leaf's left or right pointer, return nullptr
}

// deletes all of tree
template<typename KeyType, typename ValueType>
void MyMap<KeyType, ValueType>::deleteAll()
{
	if (m_rootPtr != nullptr) // only delete stuff if there's a tree to be deleted
	{
		deleteThis(m_rootPtr);
		m_size = 0; // so now it's an empty tree
		m_rootPtr = nullptr; // and so the root pointer has nothing to point to
	}
}

// recursive delete function. deletes root and its subtrees
template<typename KeyType, typename ValueType>
void MyMap<KeyType, ValueType>::deleteThis(typename MyMap<KeyType, ValueType>::Node* toBeDeleted)
{
	if (toBeDeleted == nullptr) // get out of this function call!!!
		return;

	if (toBeDeleted->left != nullptr)
		deleteThis(toBeDeleted->left); // delete the left subtree
	if (toBeDeleted->right != nullptr)
		deleteThis(toBeDeleted->right); // delete the right subtree

	m_size--;
	delete toBeDeleted; // both subtrees have been deleted, so delete the node
}

#endif // for MY_MAP