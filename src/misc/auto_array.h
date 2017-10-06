#ifndef __auto_array_H__
#define __auto_array_H__

// like auto_ptr, is an array of pointers of the type that will get freed at destruction
#include <vector>

namespace std {

template<typename T> class auto_array : public vector<T *>
{
public:
	auto_array(unsigned n_elements)
	{
		for(unsigned t=0;t<n_elements;t++)
			this->push_back((T *)NULL);
	}

	virtual ~auto_array()
	{
		for(unsigned t=0;t<this->size();t++)
			delete this->operator[](t);
	}

};

/*
template<typename T> class auto_array
{
public:
	auto_array(unsigned n_elements)
	{
		for(unsigned t=0;t<n_elements;t++)
			v.push_back(NULL);
	}

	virtual ~auto_array()
	{
		for(unsigned t=0;t<v.size();t++)
			delete v[t];
	}

	T * operator[](unsigned index) const 
	{ 
		return v[index]; 
	}

private:
	vector<T *> v;
};
*/

};


#endif
