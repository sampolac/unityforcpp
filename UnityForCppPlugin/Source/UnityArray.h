//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

#ifndef UNITY_ARRAY_H
#define UNITY_ARRAY_H


//This macro is used to define the supported types. Ideally, you shouldn't use any type beyond the
//blittable types (defined at UnityArray.cpp) and structs containing only these types (individually, not in arrays)
#define UA_SUPPORTED_TYPE(cpp_type, cs_name) \
	template<> const char* const UnityForCpp::UnityArray<cpp_type>::s_cppTypeName = #cpp_type; \
	template<> const char* const UnityForCpp::UnityArray<cpp_type>::s_managedTypeName = cs_name;

namespace UnityForCpp
{	
	//required foward declaration
	namespace UnityAdapter {
		namespace Internals {
			struct DeliveredManagedArray;
		}
	}

//Non-template base class for our UnityArray template class, a pointer to it may hold any instance of its UnityArray<> subclasses
//This is an ABSTRACT class, instance UnityArray<TYPE> instead. You may delete an instances from a pointer to UnityArrayBase.
class UnityArrayBase
{
public:
	//YOU MUST CALL THIS METHOD BEFORE USING AN UnityArray, unless you received it already allocated (GetId >= 0).
	//length is expressed in number of items for the specific type of the array.
	void Alloc(int length);

	//It is called automatically on destruction, YOU SHOULD CALL IT YOURSELF ONLY IF YOUR APPLICATION KEEPS THE UnityArray INSTANCE  
	//BEYOND THE OnDestroy METHOD CALL. In this case you need to release the shared array when OnDestroy is called. Release  
	//does that preserving the instance. After Release is called the instance can still be used if Alloc is called on a new game run.
	void Release();

	//Get the array id, being in common with the id at the C# side, so pass it to C# and access this array from there too
	int GetId() const { return m_id; }

	//array size (number of items)
	int GetLength() const { return m_length; }

	void *GetVoidPtr() { return m_pArray; }
	const void *GetVoidPtr() const { return m_pArray; }

	//"sizeof" for the type T of UnityArray<T>
	virtual int GetTypeSize() const = 0;

	//C++ type name, for a given UnityArray<T> subclass, not only the string, but also the ptr WILL BE ALWAYS THE SAME for all  
	//instances, so a ptr comparison can be used to check if two instances of UnityArrayBase share the same type
	virtual const char* GetCppTypeName() const = 0;

	//.NET type name, this will always be the same for a given cpp type
	virtual const char* GetManagedTypeName() const = 0;

	virtual ~UnityArrayBase();

protected:
	UnityArrayBase();
	UnityArrayBase(const UnityArrayBase& unityArray); //auxiliar method on base class, used on UnityArray class
	UnityArrayBase(UnityArrayBase&& unityArray); //auxiliar method on base class, used on UnityArray class
	UnityArrayBase(int id, int length, void* pArray); //auxiliar method on base class, used on UnityArray class

	UnityArrayBase& operator=(const UnityArrayBase& unityArray) { return *this; } //REGULAR ASSIGNMENT NOT ALLOWED, copy it yourself
	void MoveAssignmentAux(UnityArrayBase& unityArray); //auxiliar method on base class, used on UnityArray class

	int m_id;
	int m_length;
	void* m_pArray;
};

//Class to wrap shared arrays between C++ and C#. 
//Preferably use it as value by aggregation, it properly supports "moving" through move constructor and assignement operator. 
//You must call "Alloc" before using it and call "Release" when OnDestroy happens for your app and the instance still alives. 
//FOR CHECKING/EXTENDING THE SUPPORTED TYPES look for the "UA_SUPPORTED_TYPE" macro usage at UnityArray.cpp
template <typename T>
class UnityArray : public UnityArrayBase
{
public:
	//cpp type name as static const variable, its ptr value never changes for a given C++ type
	static const char* const s_cppTypeName;

	//C# type name as static const variable, its ptr value never changes for a given C++ type
	static const char* const s_managedTypeName;

	virtual int GetTypeSize() const { return sizeof(T); }
	virtual const char* GetCppTypeName() const { return s_cppTypeName; }
	virtual const char* GetManagedTypeName() const { return s_managedTypeName; }

	T* GetPtr() { return reinterpret_cast<T*>(m_pArray); }
	const T* GetPtr() const { return reinterpret_cast<T*>(m_pArray); }

	const T& Get(int i) const
	{
		ASSERT(m_pArray && i >= 0 && i < m_length);
		return GetPtr()[i];
	}

	T& Get(int i)
	{
		ASSERT(m_pArray && i >= 0 && i < m_length);
		return GetPtr()[i];
	}

	const T& operator[](int i) const { return Get(i); }
	T& operator[](int i) { return Get(i); }

	//Copy and Move constructors
	UnityArray() : UnityArrayBase() {}
	UnityArray(const UnityArray<T>& unityArray) : UnityArrayBase(unityArray) { }
	UnityArray(const UnityArray<T>&& unityArray) : UnityArrayBase(unityArray) { }

	//Move assignment operator, allows UnityArray<T> to be instanced on the stack and returned as parameter properly 
	UnityArray<T>& operator=(UnityArray<T>&& unityArray) 
	{ 
		MoveAssignmentAux(unityArray);
		return *this; 
	}

private:
	//DeliveredManagedArray struct may use this constructor for dropping newly delivered arrays directly on a UnityArray instance
	UnityArray(int id, int length, void* pArray)
		: UnityArrayBase(id, length, pArray) {};

	UnityArray<T>& operator=(const UnityArray<T>& unityArray) { return *this; } //NOT ALLOWED, copy yourself instead

	friend struct UnityAdapter::Internals::DeliveredManagedArray;
};

} //UnityForCpp namespace



#endif