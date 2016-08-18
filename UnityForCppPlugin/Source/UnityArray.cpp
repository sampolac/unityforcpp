//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

#include "Shared.h"
#include "UnityArray.h"
#include "UnityAdapter.h"
#include <string>

namespace UnityForCpp
{
	//All the blittable types are supported by default (https://msdn.microsoft.com/en-us/library/75dwhxf7.aspx), 
	//you may also use the macro UA_SUPPORTED_TYPE on your cpp file to support structs composed ONLY by blittable types
	UA_SUPPORTED_TYPE(uint8, "System.Byte")
	UA_SUPPORTED_TYPE(int8, "System.SByte")
	UA_SUPPORTED_TYPE(int16, "System.Int16")
	UA_SUPPORTED_TYPE(uint16, "System.UInt16")
	UA_SUPPORTED_TYPE(int32, "System.Int32")
	UA_SUPPORTED_TYPE(uint32, "System.UInt32")
	UA_SUPPORTED_TYPE(int64, "System.Int64")
	UA_SUPPORTED_TYPE(uint64, "System.UInt64")
	UA_SUPPORTED_TYPE(float, "System.Single")
	UA_SUPPORTED_TYPE(double, "System.Double")

	UnityArrayBase::UnityArrayBase()
	: m_id(-1), m_length(0), m_pArray(NULL) {}

	UnityArrayBase::UnityArrayBase(const UnityArrayBase& unityArray)
		: m_id(-1), m_length(0), m_pArray(NULL)
	{
		m_id = UnityAdapter::NewManagedArray(unityArray.GetManagedTypeName(), unityArray.GetLength(), &m_pArray);
		m_length = m_length;

		memcpy(m_pArray, unityArray.GetVoidPtr(), unityArray.GetLength()*unityArray.GetTypeSize());
	}

	UnityArrayBase::UnityArrayBase(UnityArrayBase&& unityArray)
		: m_id(unityArray.m_id), m_length(unityArray.m_length), m_pArray(unityArray.m_pArray)
	{
		unityArray.m_id = -1;
		unityArray.m_length = 0;
		unityArray.m_pArray = NULL;
	}

	UnityArrayBase::UnityArrayBase(int id, int length, void* pArray)
		: m_id(id), m_length(length), m_pArray(pArray) {}

	UnityArrayBase::~UnityArrayBase()
	{
		Release();
	}

	void UnityArrayBase::MoveAssignmentAux(UnityArrayBase& unityArray)
	{
		ASSERT(m_pArray == NULL);

		m_id = unityArray.m_id;
		m_length = unityArray.m_length;
		m_pArray = unityArray.m_pArray;;

		unityArray.m_id = -1;
		unityArray.m_length = 0;
		unityArray.m_pArray = NULL;
	}

	void UnityArrayBase::Alloc(int length)
	{
		ASSERT(m_pArray == NULL);
		m_id = UnityAdapter::NewManagedArray(GetManagedTypeName(), length, &m_pArray);
		m_length = length;
	}

	void UnityArrayBase::Release()
	{
		if (m_pArray == NULL)
			return;
		
		UnityAdapter::ReleaseManagedArray(m_id);
		m_id = -1;
		m_length = 0;
		m_pArray = NULL;
	}
}
