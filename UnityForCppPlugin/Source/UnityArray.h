//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#ifndef UNITY_ARRAY_H
#define UNITY_ARRAY_H

namespace UnityForCpp
{
	//required foward declaration
	namespace UnityAdapter {
		namespace Internals {
			struct DeliveredManagedArray;
		}
	}

//Non-template base class for our UnityArray template class, a pointer to it may hold any instance of its UnityArray<> subclasses
//This is an ABSTRACT class, instance UnityArray<TYPE> instead. You may delete an instance of it from a pointer to UnityArrayBase.
class UnityArrayBase
{
public:
	//Get the array id, being in common with the id at the C# side, so pass it to C# and access this array from there too
	int GetId() const { return m_id; }

	//array size (number of items)
	int GetCount() const { return m_count; }

	//C++ type name, for a given UnityArray<T> subclass, not only the string, but also the ptr WILL BE ALWAYS THE SAME for all  
	//instances, so a ptr comparison can be used to check if two instances of UnityArrayBase share the same type
	virtual const char* GetCppTypeName() const = 0;

	//.NET type name, this will always be the same for a given cpp type
	virtual const char* GetManagedTypeArray() const = 0;

	virtual ~UnityArrayBase();

protected:
	UnityArrayBase(const char* managedTypeName, int count);
	UnityArrayBase(int id, int count, void* pArray);

	int m_id;
	int m_count;
	void* m_pArray;
};

//Class to wrap shared arrays between C++ and C#. You may instance it directly with the "new" operator.
//ONCE INSTANCED OWENERSHIP OF THE INSTANCE MUST BE STABLISHED AND THE IT MUST BE PROPERLY DELETED WITH "delete" (or "DELETE" macro)
//Your code should delete all the instances of UnityArray when the game is stopped from the Unity Editor in order to start a
//new gameplay without remaining allocated memory from previous plays.
//FOR CHECKING/EXTENDING THE SUPPORTED TYPES look for the "UA_SUPPORTED_TYPE" macro usage at UnityArray.cpp
template <typename T>
class UnityArray : public UnityArrayBase
{
public:
	//cpp type name as static const variable, its ptr value never changes for a given C++ type
	static const char* const s_cppTypeName;

	//C# type name as static const variable, its ptr value never changes for a given C++ type
	static const char* const s_managedTypeName;

	//Use this constructor with the "new" operator for instance a new UnityArray with the size "count"
	UnityArray(int count)
		: UnityArrayBase(s_managedTypeName, count) {}

	virtual const char* GetCppTypeName() const { return s_cppTypeName; }
	virtual const char* GetManagedTypeArray() const { return s_managedTypeName; }

	T* GetArrayPtr() { return reinterpret_cast<T*>(m_pArray); }

	const T& operator[](int i) const
	{
		ASSERT(i >= 0 && i < m_count);
		return GetArrayPtr()[i];
	}

	T& operator[](int i)
	{
		ASSERT(i >= 0 && i < m_count);
		return GetArrayPtr()[i];
	}

private:
	//DeliveredManagedArray struct may use this constructor for dropping newly delivered arrays directly on a UnityArray instance
	UnityArray(int id, int count, void* pArray)
		: UnityArrayBase(id, count, pArray) {};

	friend struct UnityAdapter::Internals::DeliveredManagedArray;
};

} //UnityForCpp namespace



#endif