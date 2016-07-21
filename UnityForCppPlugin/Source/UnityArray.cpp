//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#include "Shared.h"
#include "UnityArray.h"
#include "UnityAdapter.h"


namespace UnityForCpp
{

//Use this macro for supporting new array types (https://msdn.microsoft.com/en-us/library/ya5y69ds.aspx).
//IMPORTANT: Be sure about type compatibility between C# and your required platforms. A type correspondency 
//that works well on one platform may not work at other platform. 
//For Visual C++ types correspondency check https://msdn.microsoft.com/en-us/library/0wf2yk2k.aspx
#define UA_SUPPORTED_TYPE(cpp_type, cs_name) \
	template<> const char* const UnityArray<cpp_type>::s_cppTypeName = #cpp_type; \
	template<> const char* const UnityArray<cpp_type>::s_managedTypeName = #cs_name;

	//Currently supported array types
	UA_SUPPORTED_TYPE(uchar, System.Byte)
	UA_SUPPORTED_TYPE(int, System.Int32)
	UA_SUPPORTED_TYPE(uint, System.UInt32)
	UA_SUPPORTED_TYPE(float, System.Single)

	UnityArrayBase::UnityArrayBase(const char* managedTypeName, int count)
		: m_count(count)
	{
		m_id = UnityAdapter::NewManagedArray(managedTypeName, count, &m_pArray);
	}

	UnityArrayBase::UnityArrayBase(int id, int count, void* pArray)
		: m_id(id), m_count(count), m_pArray(pArray) {}

	UnityArrayBase::~UnityArrayBase()
	{
		UnityAdapter::ReleaseManagedArray(GetId());
	}

}