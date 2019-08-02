/**----------------------------------------------------------------------------
 * PROJECT: kbddrv
 * PURPOSE:
 * Simple double linked list with garbage collection handling
 *-----------------------------------------------------------------------------  
 * CREATION: May 15, 2013
 * Author: Luca Mini
 * 
 * LICENCE: please see LICENCE.TXT file
 * 
 * HISTORY (of the module):
 *-----------------------------------------------------------------------------
 * Author              | Date        | Description
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

#ifndef SIMPLELINKLIST_H_
#define SIMPLELINKLIST_H_

#include <cstdlib>
#include <iostream>


/*
 * Main template class
 */
template<class T>
class SimpleLinkList
{
public:
	/**
	 * data structure
	 */
	typedef struct node_st
	{
	T data;
	node_st *prev;
	node_st *next;

	node_st()
		{
		prev=(node_st*)NULL;
		next=(node_st*)NULL;
		}
	} node_t;

	/**
	 * ctor
	 * @param allocNElem first N allocations
	 */
	SimpleLinkList(int allocNElem)
	{
	node_t *ep=(node_t*)NULL;
	this->allocNElem=allocNElem;
	for(int i=0;i<this->allocNElem;i++)
		{
		node_t *e=new node_t();
		e->prev=ep;
		if(ep!=NULL) ep->next=e;
		if(i==0)
			{
			firstEmpty=e;	// at the very beginning every element is part of a garbage collection chain
			}
		ep=e;
		}
	tail=head=NULL;
	count=0;	// non empty count elements
	}

	/*
	 * for the "rule of three", destructor, copy constructor and copy assignment operator must be defined
	 */
	/**
	 * dtor
	 */
	virtual ~SimpleLinkList()
	{
	deallocateAll();
	}

#if 0
	/**
	 * copy constructor
	 * @param  myCopy my copy
	 */
	SimpleLinkList(const SimpleLinkList& copyMe)
		{
		SimpleLinkList(copyMe.getCount);	// allocate the memory
		for(int i=0;i<copyMe.getCount();i++)
			{
			addHeadElem(copyMe.at(i));
			}
		}

	/**
	 * copy assignment
	 * @param other object to copy
	 * @return ref to copied object
	 */
	SimpleLinkList& operator=(SimpleLinkList const &other)    // copy assignment
  {
	if(this != &other)	// to avoid self assignment
		{
		SimpleLinkList tmp(other);

		//deallocate old data
		deallocateAll();

		// replace with copied one
		tail=tmp.tail;
		head=tmp.head;
		firstEmpty=tmp.fristEmpty;
		count=tmp.count;
		allocNElem=tmp.allocNElem;
		}
	// by convention, always return *this
	return *this;
  }
#endif
	//=============================================================================

	/**
	 * push an element on the head, so the new element became the last
	 * @param data
	 */
	void addHeadElem(T data)
	{
	node_t *e;

	if(firstEmpty!=NULL)	// use the garbage collection
		{
		e=firstEmpty;
		firstEmpty=firstEmpty->next;
		}
	else
		{
		// need another element
		e=new node_t();
		allocNElem++;
		}
	e->data=data;	// store data
	e->prev=head;
	e->next=NULL;
	head=e;
	if(e->prev!=NULL) e->prev->next=e;
	if(tail==NULL) tail=e; // the first element
	count++;
	}

	/**
	 * push an element on the tail. the actual first element became the second one
	 * @param data
	 */
	void addTailElem(T data)
	{
	node_t *e;

	if(firstEmpty!=NULL)	// use the garbage collection
		{
		e=firstEmpty;
		firstEmpty=firstEmpty->next;
		}
	else
		{
		// need another element
		e=new node_t();
		allocNElem++;
		}
	e->data=data;	// store data
	e->prev=NULL;
	e->next=tail;
	tail=e;
	if(e->next!=NULL) e->next->prev=e;
	if(head==NULL) head=e; // the first element
	count++;
	}

	/**
	 * remove all elements that are equal to data. It iterate over all the elements
	 * @param data data to remove
	 */
	void removeElem(T data)
	{
	if(count>0)
		{
		node_t *e=tail;
		node_t *tmp;
		while(e!=NULL)
			{
			if(e->data==data)
				{
				// remove element from the chain
				e->prev->next=e->next;
				e->next->prev=e->prev;
				tmp=e->next;
				if(e==tail) tail=e->next;
				if(e==head) tail=e->prev;

				// put to the tail of the garbage collection chain
				appendToGarbageCollection(e);

				e=tmp;	// set to the next

				count--;
				}
			else
				{
				e=e->next;
				}
			}
		}
	}


	/**
	 * get the head value
	 * @return head value
	 */
	T getHead()
	{
	return head->data;
	}

	/**
	 * get the tail value
	 * @return tail value
	 */
	T getTail()
	{
	return tail->data;
	}

	/**
	 * remove the head node
	 * Usefull after @see getHead
	 */
	void popHead()
	{
	node_t *e=head;
	// set new head
	head=head->prev;
	// put to garbage collection
	appendToGarbageCollection(e);
	}


	/**
	 * remove the tail node
	 * Usefull after @see getTail
	 */
	void popTail()
	{
	node_t *e=tail;
	// set new head
	tail=tail->next;
	// put to garbage collection
	appendToGarbageCollection(e);
	}

	/**
	 * get the number of elements in the list
	 * @return number of elements
	 */
	int getCount()
	{
	return count;
	}

	/**
	 * overload of the [] to get access at a single element
	 * @param i index
	 * @return element value at i index
	 */
	T& at(int i)
	{
	node_t *e;
	// traverse all the chain, starting from the nearest end
	if(i>(count>>1)) // near to the end
		{
		e=head;
		for(int j=count-1;j>i;j--)
			{
			e=e->prev;
			}
		return e->data;
		}
	else	// tear the beginning
		{
		e=tail;
		for(int j=0;j<i;j++)
			{
			e=e->next;
			}
		return e->data;
		}
	}

	/**
	 * overload of the [] to get access at a single element
	 * @param i index
	 * @return element value at i index
	 */
	T& operator[] (const int& i)
	{
	return at(i);
	}

	/**
	 * tell if list is empty
	 * @return true: empty; false: not empty
	 */
	bool isEmpty()
	{
	return (count==0) ? (true) : (false);
	}

private:
	int allocNElem,count;
	node_t *tail,*head;
	node_t *firstEmpty; // handle the garbage collection (empty elements)

	/*
	 * rule of three satisfaction :)
	 * to avoid copying
	 */
	SimpleLinkList(const SimpleLinkList&);
	SimpleLinkList& operator=(const SimpleLinkList&);


	/**
	 * append an element to hte garbage collection list
	 * @param e element to collect
	 */
	void appendToGarbageCollection(node_t *e)
	{
	e->next=firstEmpty;
	e->prev=NULL;
	firstEmpty=e;
	}

	void deallocateAll()
	{
	int c1=0,c2=0;
	node_t *e,*etmp;
	// clean data
	e=tail;
	while(e!=NULL)
		{
		etmp=e->next;
		delete e;
		e=etmp;
		c1++;
		}
	//std::cout << "deleted "<< c1 << std::endl;
	// clean garbage
	e=firstEmpty;
	while(e!=NULL)
		{
		etmp=e->next;
		delete e;
		e=etmp;
		c2++;
		}
	//std::cout << "deleted "<< c2 << std::endl;
	if((c1+c2)!=allocNElem)
		{
		std::cerr << "ERROR SimpleLinkList: deallocate " << c1+c2 << " elements but " << allocNElem << " was allocated" << std::endl;
		}
	}
};

//-----------------------------------------------
#endif /* SIMPLELINKLIST_H_ */
