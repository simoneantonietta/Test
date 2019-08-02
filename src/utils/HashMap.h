/**----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE:
 * generic hash map data structure
 *-----------------------------------------------------------------------------  
 * CREATION: 25 Oct 2013
 * Author: Luca Mini
 * 
 * LICENCE: please see LICENCE.TXT file
 * 
 * HISTORY (of the module):
 *-----------------------------------------------------------------------------
 * Author              | Date        | Description
 *-----------------------------------------------------------------------------
 * Luca Mini             25/10/2013    added a specialized hash func. optimized for integer keys
 *
 *-----------------------------------------------------------------------------
 */

#ifndef HASHMAP_H_
#define HASHMAP_H_

// ============================================================================
//     Author: K Perkins
//     Date:   June 11, 2013
//     Taken From: http://programmingnotes.freeweq.com/
//     File:  HashMap.h
//     Description: This is a class which implements various functions
//          demonstrating the use of a Hash Map.
// ============================================================================

#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>

// if user doesnt define, this is the
// default hash map size
const int HASH_SIZE = 256;

template<class Key, class Value>
class HashMap
{
public:
	HashMap(int hashSze = HASH_SIZE);
	/*   Function: Constructor initializes hash map
	 Precondition: None
	 Postcondition: Defines private variables */
	bool IsEmpty(int keyIndex);
	/*   Function: Determines whether hash map is empty at the given hash
	 map key index
	 Precondition: Hash map has been created
	 Postcondition: The function = true if the hash map is empty and the
	 function = false if hash map is not empty */
	bool IsFull();
	/*   Function: Determines whether hash map is full
	 Precondition: Hash map has been created
	 Postcondition: The function = true if the hash map is full and the
	 function = false if hash map is not full */
	int Hash(Key m_key);
	/*   Function: Computes and returns a hash map key index for a given item
	 The returned key index is the given cell where the item resides
	 Precondition:  Hash map has been created and is not full
	 Postcondition: The hash key is returned */

	Value& Get(Key m_key);

	void Insert(Key m_key, Value m_value);
	/*   Function: Adds new item to the back of the list at a given key in the hash map
	 A unique hash key is automatically generated for each new item
	 Precondition:  Hash map has been created and is not full
	 Postcondition: Item is in the hash map */
	bool Remove(Key m_key, Value deleteItem);
	/*   Function: Removes the first instance from the map whose value is "deleteItem"
	 Precondition:  Hash map has been created and is not empty
	 Postcondition: The function = true if deleteItem is found and the
	 function = false if deleteItem is not found */
	void Sort(int keyIndex);
	/*   Function: Sort the items in the map at the given hashmap key index
	 Precondition: Hash map has been initialized
	 Postcondition: The hash map is sorted */
	int TableSize();
	/*   Function: Return the size of the hash map
	 Precondition: Hash map has been initialized
	 Postcondition: The size of the hash map is returned */
	int TotalElems();
	/*   Function: Return the total number of elements contained in the hash map
	 Precondition: Hash map has been initialized
	 Postcondition: The size of the hash map is returned */
	int BucketSize(int keyIndex);
	/*   Function: Return the number of items contained in the hash map
	 cell at the given hashmap key index
	 Precondition: Hash map has been initialized
	 Postcondition: The size of the given key cell is returned */
	int Count(Key m_key, Value searchItem);
	/*   Function: Return the number of times searchItem appears in the map
	 at the given key
	 Precondition: Hash map has been initialized
	 Postcondition: The number of times searchItem appears in the map is returned */
	int ContainsKey(Key m_key);
	/*   Function: Return the number of times the given key appears in the hashmap
	 Precondition: Hash map has been initialized
	 Postcondition: The number of times the given key appears in the map is returned */
	void MakeEmpty();
	/*   Function: Initializes hash map to an empty state
	 Precondition: Hash map has been created
	 Postcondition: Hash map no longer exists */
	~HashMap();
	/*   Function: Removes the hash map
	 Precondition: Hash map has been declared
	 Postcondition: Hash map no longer exists */

	//  -- ITERATOR CLASS --
	class Iterator;
	/*   Function: Class declaration to the iterator
	 Precondition: Hash map has been declared
	 Postcondition: Hash Iterator has been declared */

	Iterator begin(int keyIndex)
	{
	return (!IsEmpty(keyIndex)) ? head[keyIndex] : NULL;
	}
	/*   Function: Returns the beginning of the current hashmap key index
	 Precondition: Hash map has been declared
	 Postcondition: Hash cell has been returned to the Iterator */

	Iterator end(int keyIndex = 0)
	{
	return NULL;
	}
	/*   Function: Returns the end of the current hashmap key index
	 Precondition: Hash map has been declared
	 Postcondition: Hash cell has been returned to the Iterator */

private:
	struct KeyValue  // struct to hold key/value pairs
	{
		Key key;
		Value value;
	};
	struct node
	{
		KeyValue currentItem;
		node* next;
	};
	node** head;  // array of linked list declaration - front of each hash map cell
	int hashSize;  // the size of the hash map (how many cells it has)
	int totElems;  // holds the total number of elements in the entire table
	int* bucketSize;  // holds the total number of elems in each specific hash map cell
};

//=========================  Implementation  ================================//

template<class Key, class Value>
HashMap<Key, Value>::HashMap(int hashSze)
{
hashSize = hashSze;
head = new node*[hashSize];
bucketSize = new int[hashSize];
for (int x = 0; x < hashSize; ++x)
	{
	head[x] = NULL;
	bucketSize[x] = 0;
	}
totElems = 0;
}/* End of HashMap */

template<class Key, class Value>
bool HashMap<Key, Value>::IsEmpty(int keyIndex)
{
if (keyIndex >= 0 && keyIndex < hashSize)
	{
	return head[keyIndex] == NULL;
	}
return true;
}/* End of IsEmpty */

template<class Key, class Value>
bool HashMap<Key, Value>::IsFull()
{
try
	{
	node* location = new node;
	delete location;
	return false;
	}
catch (std::bad_alloc&)
	{
	return true;
	}
}/* End of IsFull */

template<class Key, class Value>
int HashMap<Key, Value>::Hash(Key m_key)
{
long h = 19937;
std::stringstream convert;

// convert the parameter to a string using "stringstream" which is done
// so we can hash multiple datatypes using only one function
convert << m_key;
std::string temp = convert.str();

for (unsigned x = 0; x < temp.length(); ++x)
	{
	h = (h << 6) ^ (h >> 26) ^ temp[x];
	}
return abs(h % hashSize);
} /* End of Hash */

/**
 * specialized version for a faster hash function if the key is an integer
 * @param m_key
 * @return
 */
//template<class Key, class Value>
//int HashMap<Key, Value>::Hash<int, Value>(Key<int> m_key)
//{
//return m_key % hashSize;
//}

template<class Key, class Value>
void HashMap<Key, Value>::Insert(Key m_key, Value m_value)
{
if (IsFull())
	{
	//std::cout<<"\nINSERT ERROR - HASH MAP FULL\n";
	}
else
	{
	int keyIndex = Hash(m_key);
	node* newNode = new node;   // add new node
	newNode->currentItem.key = m_key;
	newNode->currentItem.value = m_value;
	newNode->next = NULL;

	if (IsEmpty(keyIndex))
		{
		head[keyIndex] = newNode;
		}
	else
		{
		node* temp = head[keyIndex];
		while (temp->next != NULL)
			{
			temp = temp->next;
			}
		temp->next = newNode;
		}
	++bucketSize[keyIndex];
	++totElems;
	}
}/* End of Insert */

template<class Key, class Value>
bool HashMap<Key, Value>::Remove(Key m_key, Value deleteItem)
{
bool isFound = false;
node* temp;
int keyIndex = Hash(m_key);

if (IsEmpty(keyIndex))
	{
	//std::cout<<"\nREMOVE ERROR - HASH MAP EMPTY\n";
	}
else if (head[keyIndex]->currentItem.key == m_key
    && head[keyIndex]->currentItem.value == deleteItem)
	{
	temp = head[keyIndex];
	head[keyIndex] = head[keyIndex]->next;
	delete temp;
	--totElems;
	--bucketSize[keyIndex];
	isFound = true;
	}
else
	{
	for (temp = head[keyIndex]; temp->next != NULL; temp = temp->next)
		{
		if (temp->next->currentItem.key == m_key
		    && temp->next->currentItem.value == deleteItem)
			{
			node* deleteNode = temp->next;
			temp->next = temp->next->next;
			delete deleteNode;
			isFound = true;
			--totElems;
			--bucketSize[keyIndex];
			break;
			}
		}
	}
return isFound;
}/* End of Remove */

template<class Key, class Value>
void HashMap<Key, Value>::Sort(int keyIndex)
{
if (IsEmpty(keyIndex))
	{
	//std::cout<<"\nSORT ERROR - HASH MAP EMPTY\n";
	}
else
	{
	int listSize = BucketSize(keyIndex);
	bool sorted = false;

	do
		{
		sorted = true;
		int x = 0;
		for (node* temp = head[keyIndex]; temp->next != NULL && x < listSize - 1;
		    temp = temp->next, ++x)
			{
			if (temp->currentItem.value > temp->next->currentItem.value)
				{
				std::swap(temp->currentItem, temp->next->currentItem);
				sorted = false;
				}
			}
		--listSize;
		}
	while (!sorted);
	}
}/* End of Sort */

template<class Key, class Value>
int HashMap<Key, Value>::TableSize()
{
return hashSize;
}/* End of TableSize */

template<class Key, class Value>
int HashMap<Key, Value>::TotalElems()
{
return totElems;
}/* End of TotalElems */

template<class Key, class Value>
int HashMap<Key, Value>::BucketSize(int keyIndex)
{
return (!IsEmpty(keyIndex)) ? bucketSize[keyIndex] : 0;
}/* End of BucketSize */

template<class Key, class Value>
int HashMap<Key, Value>::Count(Key m_key, Value searchItem)
{
int keyIndex = Hash(m_key);
int search = 0;

if (IsEmpty(keyIndex))
	{
	//std::cout<<"\nCOUNT ERROR - HASH MAP EMPTY\n";
	}
else
	{
	for (node* temp = head[keyIndex]; temp != NULL; temp = temp->next)
		{
		if (temp->currentItem.key == m_key && temp->currentItem.value == searchItem)
			{
			++search;
			}
		}
	}
return search;
}/* End of Count */

template<class Key, class Value>
int HashMap<Key, Value>::ContainsKey(Key m_key)
{
int keyIndex = Hash(m_key);
int search = 0;

if (IsEmpty(keyIndex))
	{
	//std::cout<<"\nCONTAINS KEY ERROR - HASH MAP EMPTY\n";
	}
else
	{
	for (node* temp = head[keyIndex]; temp != NULL; temp = temp->next)
		{
		if (temp->currentItem.key == m_key)
			{
			++search;
			}
		}
	}
return search;
}/* End of ContainsKey */

/**
 * return the value directly, usefull for unique key maps
 * @param m_key
 * @return value
 */
template<class Key, class Value>
Value& HashMap<Key, Value>::Get(Key m_key)
{
int keyIndex = Hash(m_key);


if (IsEmpty(keyIndex))
	{
	//std::cout<<"\nCONTAINS KEY ERROR - HASH MAP EMPTY\n";
	}
else
	{
	node* temp = head[keyIndex];
	return temp;
	}
return NULL;
}


template<class Key, class Value>
void HashMap<Key, Value>::MakeEmpty()
{
totElems = 0;
for (int x = 0; x < hashSize; ++x)
	{
	if (!IsEmpty(x))
		{
		//std::cout << "Destroying nodes ...\n";
		while (!IsEmpty(x))
			{
			node* temp = head[x];
			//std::cout << temp-> currentItem.value <<std::endl;
			head[x] = head[x]->next;
			delete temp;
			}
		}
	bucketSize[x] = 0;
	}
}/* End of MakeEmpty */

template<class Key, class Value>
HashMap<Key, Value>::~HashMap()
{
MakeEmpty();
delete[] head;
delete[] bucketSize;
}/* End of ~HashMap */

//   END OF THE HASH MAP CLASS
// -----------------------------------------------------------
//   START OF THE HASH MAP ITERATOR CLASS

template<class Key, class Value>
class HashMap<Key, Value>::Iterator: public std::iterator<
    std::forward_iterator_tag, Value>, public HashMap<Key, Value>
{
public:
	// Iterator constructor
	Iterator(node* otherIter = NULL)
	{
	itHead = otherIter;
	}
	~Iterator()
	{
	}
	// The assignment and relational operators are straightforward
	Iterator& operator=(const Iterator& other)
	{
	itHead = other.itHead;
	return (*this);
	}
	bool operator==(const Iterator& other) const
	{
	return itHead == other.itHead;
	}
	bool operator!=(const Iterator& other) const
	{
	return itHead != other.itHead;
	}
	bool operator<(const Iterator& other) const
	{
	return itHead < other.itHead;
	}
	bool operator>(const Iterator& other) const
	{
	return other.itHead < itHead;
	}
	bool operator<=(const Iterator& other) const
	{
	return (!(other.itHead < itHead));
	}
	bool operator>=(const Iterator& other) const
	{
	return (!(itHead < other.itHead));
	}
	// Update my state such that I refer to the next element in the
	// HashMap.
	Iterator operator+(int incr)
	{
	node* temp = itHead;
	for (int x = 0; x < incr && temp != NULL; ++x)
		{
		temp = temp->next;
		}
	return temp;
	}
	Iterator operator+=(int incr)
	{
	for (int x = 0; x < incr && itHead != NULL; ++x)
		{
		itHead = itHead->next;
		}
	return itHead;
	}
	Iterator& operator++()  // pre increment
	{
	if (itHead != NULL)
		{
		itHead = itHead->next;
		}
	return (*this);
	}
	Iterator operator++(int)  // post increment
	{
	node* temp = itHead;
	this->operator++();
	return temp;
	}
	KeyValue& operator[](int incr)
	{
	// Return "junk" data
	// to prevent the program from crashing
	if (itHead == NULL || (*this + incr) == NULL)
		{
		return junk;
		}
	return (*(*this + incr));
	}

	// Return a reference to the value in the node.  I do this instead
	// of returning by value so a caller can update the value in the
	// node directly.
	KeyValue& operator*()
	{
	// Return "junk" data
	// to prevent the program from crashing
	if (itHead == NULL)
		{
		return junk;
		}
	return itHead->currentItem;
	}
	KeyValue* operator->()
	{
	return (&**this);
	}
private:
	node* itHead;
	KeyValue junk;
};

#if 0
// USAGE EXAMPLE
// DEMONSTRATE BASIC USE AND THE REMOVE / SORT FUNCTIONS
#include <iostream>
#include <string>
#include "../../WUtilities/HashMap.h"
using namespace std;

// iterator declaration
typedef HashMap<string, int>::Iterator iterDec;

int main()
	{
	// declare variables
	HashMap<string, int> hashMap;

	// place items into the hash map using the 'insert' function
	// NOTE: its OK for dupicate keys to be inserted into the hash map
	hashMap.Insert("BIOL", 585);
	hashMap.Insert("CPSC", 386);
	hashMap.Insert("ART", 101);
	hashMap.Insert("CPSC", 462);
	hashMap.Insert("HIST", 251);
	hashMap.Insert("CPSC", 301);
	hashMap.Insert("MATH", 270);
	hashMap.Insert("PE", 145);
	hashMap.Insert("BIOL", 134);
	hashMap.Insert("GEOL", 201);
	hashMap.Insert("CIS", 465);
	hashMap.Insert("CPSC", 240);
	hashMap.Insert("GEOL", 101);
	hashMap.Insert("MATH", 150);
	hashMap.Insert("DANCE", 134);
	hashMap.Insert("CPSC", 131);
	hashMap.Insert("ART", 345);
	hashMap.Insert("CHEM", 185);
	hashMap.Insert("PE", 125);
	hashMap.Insert("CPSC", 120);

	// display the number of times the key "CPSC" appears in the hashmap
	cout<<"The key 'CPSC' appears in the hash map "<<
	hashMap.ContainsKey("CPSC")<<" time(s)\n";

	// declare an iterator for the "CPSC" key so we can display data to screen
	iterDec it = hashMap.begin(hashMap.Hash("CPSC"));

	// display the first value
	cout<<"\nThe first item with the key 'CPSC' is: "
	<<it[0].value<<endl;

	// display all the values in the hash map whose key matches "CPSC"
	// NOTE: its possible for multiple different keys types
	//   to be placed into the same hash map bucket
	cout<<"\nThese are all the items in the hash map whose key is 'CPSC': \n";
	for(int x=0; x < hashMap.BucketSize(hashMap.Hash("CPSC")); ++x)
		{
		if(it[x].key == "CPSC")  // make sure this is the key we are looking for
			{
			cout<<"  Key-> "<<it[x].key<<"\tValue-> "<<it[x].value<<endl;
			}
		}

	// remove the first value from the key "CPSC"
	cout<<"\n[REMOVE THE VALUE '"<<it[0].value<<"' FROM THE KEY '"<<it[0].key<<"']\n";
	hashMap.Remove("CPSC",it[0].value);

	// display the number of times the key "CPSC" appears in the hashmap
	cout<<"\nNow the key 'CPSC' only appears in the hash map "<<
	hashMap.ContainsKey("CPSC")<<" time(s)\n";

	// update the iterator to the current hash map state
	it = hashMap.begin(hashMap.Hash("CPSC"));

	// sort the values in the hash map bucket whose key is "CSPC"
	hashMap.Sort(hashMap.Hash("CPSC"));

	// display the values whose key matches "CPSC"
	cout<<"\nThese are the sorted items in the hash map whose key is 'CPSC': \n";
	for(int x=0; x < hashMap.BucketSize(hashMap.Hash("CPSC")); ++x)
		{
		if(it[x].key == "CPSC")
			{
			cout<<"  Key-> "<<it[x].key<<"\tValue-> "<<it[x].value<<endl;
			}
		}

	// display all the key/values in the entire hash map
	cout<<"\nThese are all of the items in the entire hash map: \n";
	for(int x=0; x < hashMap.TableSize(); ++x)
		{
		if(!hashMap.IsEmpty(x))
			{
			for(iterDec iter = hashMap.begin(x); iter != hashMap.end(x); ++iter)
				{
				cout<<"  Key-> "<<(*iter).key<<"\tValue-> "<<iter->value<<endl;
				}
			cout<<endl;
			}
		}

	// display the total number of items in the hash map
	cout<<"The total number of items in the hash map is: "<<
	hashMap.TotalElems()<<endl;

	return 0;
	}  // http://programmingnotes.freeweq.com/

#endif

//-----------------------------------------------
#endif /* HASHMAP_H_ */
