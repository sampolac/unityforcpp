//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#include "UnityAdapter.h"
#include <assert.h>
#include <stdio.h>

namespace UnityForCpp
{
namespace UnityAdapter
{
namespace Internals
{

//Use this macro for supporting new array types (https://msdn.microsoft.com/en-us/library/ya5y69ds.aspx).
//IMPORTANT: Be sure about type compatibility between C# and your required platforms. A type correspondency 
//that works well on one platform may not work at other platform. 
//For Visual C++ types correspondency check https://msdn.microsoft.com/en-us/library/0wf2yk2k.aspx
#define UA_SUPPORTED_TYPE(cpp_type, cs_name) template<> const char* GetManagedTypeName<cpp_type>() { return #cs_name; }

	//Currently supported array types
	UA_SUPPORTED_TYPE(void, System.Byte)
	UA_SUPPORTED_TYPE(int, System.Int32)
	UA_SUPPORTED_TYPE(uint, System.UInt32)
	UA_SUPPORTED_TYPE(float, System.Single)

	//struct to hold the required info to manage the arrays currently shared between C# and C++
	//We make a simple linked list between instances of it using the "pNext" field.
	typedef struct _ArrayInfo
	{
		int id;
		const char* managedTypeName;
		int size;
		void* pArray;
		struct _ArrayInfo* pNext;

		_ArrayInfo()
			: id(-1), managedTypeName(NULL), size(0), pArray(NULL), pNext(NULL) {}

		_ArrayInfo(const struct _ArrayInfo& ai)
			: id(ai.id), managedTypeName(ai.managedTypeName), size(ai.size), pArray(ai.pArray), pNext(NULL) {}
	} ArrayInfo;

	//"f" prefix stands for file variable (C static variable), "g" for global variable, "n" for namaspace global variable
	//"s" for class static attribute, "m" for class instance attribute, "c" for function static variables

	static ArrayInfo*					nf_pArrayInfoLinkedListHead = NULL; //ArrayInfo linked list head

	//When the C# codes calls DeliverRequestedManagedArray, the returned C# array info is saved here
	static ArrayInfo					nf_justDeliveredArrayInfo = ArrayInfo();

	//C# function pointers to be set right after the DLL loading via the UnityAdapterPlugin interface
	static OutputDebugStrFcPtr			nf_OutputDebugStr = NULL;
	static RequestFileContentFcPtr		nf_RequestFileContent = NULL;
	static SaveTextFileFcPtr			nf_SaveTextFile = NULL;
	static RequestManagedArrayFcPtr		nf_RequestManagedArray = NULL;
	static ReleaseManagedArrayFcPtr		nf_ReleaseManagedArray = NULL;

	//If the C# code has delivered an array by calling DeliverRequestedManagedArray this method will
	//create a new ArrayInfo node and insert it on ArrayInfo simple linked list
	ArrayInfo* AcquireDeliveredArray()
	{
		if (nf_justDeliveredArrayInfo.pArray == NULL)
			return NULL;

		ArrayInfo* pNewArrayInfo = new ArrayInfo(nf_justDeliveredArrayInfo);
		nf_justDeliveredArrayInfo.pArray = NULL; //**reset it for next usage**

		//keep things simple by just inserting the new linked list node on the front of the list
		pNewArrayInfo->pNext = nf_pArrayInfoLinkedListHead;
		nf_pArrayInfoLinkedListHead = pNewArrayInfo;

		return nf_pArrayInfoLinkedListHead;
	}

	//check NewManagedArray<T>(...) declaration for comments
	int NewManagedArray(int size, const char* managedTypeName)
	{
		ASSERT(nf_RequestManagedArray);
		nf_RequestManagedArray(managedTypeName, size);
		ArrayInfo* pArrayInfo = AcquireDeliveredArray();
		ASSERT(pArrayInfo != NULL);
		pArrayInfo->managedTypeName = managedTypeName;
		return pArrayInfo->id;
	}

	//check GetManagedArray<T>(...) declaration for comments
	void* GetManagedArray(int arrayId, int* pSizeOutput, const char* managedTypeName)
	{
		ArrayInfo* pArrayInfo = nf_pArrayInfoLinkedListHead;
		while (pArrayInfo != NULL)
		{
			if (pArrayInfo->id == arrayId)
			{
				ASSERT(managedTypeName == pArrayInfo->managedTypeName); //assure type safety by comparing static string addresses
				if (pSizeOutput)
					*pSizeOutput = pArrayInfo->size;

				return pArrayInfo->pArray;
			}

			pArrayInfo = pArrayInfo->pNext;
		}

		return NULL;
	}


	//----------------


	void SetOutputDebugStrFcPtr(OutputDebugStrFcPtr fcPtr)
	{
		nf_OutputDebugStr = fcPtr;
	}

	void SetFileFcPtrs(RequestFileContentFcPtr requestFileContentFcPtr,
		SaveTextFileFcPtr openAndWriteAllFileFcPtr)
	{
		nf_RequestFileContent = requestFileContentFcPtr;
		nf_SaveTextFile = openAndWriteAllFileFcPtr;
	}


	void SetArrayFcPtrs(RequestManagedArrayFcPtr requestManagedArrayFcPtr,
		ReleaseManagedArrayFcPtr releaseManagedArrayFcPtr)
	{
		nf_RequestManagedArray = requestManagedArrayFcPtr;
		nf_ReleaseManagedArray = releaseManagedArrayFcPtr;
	}

	void DeliverRequestedManagedArray(int id, void* pArray, int size)
	{
		if (!pArray)
			return;

		//after this function has returned and the enclosing C# function has also returned
		//AcquireDeliveredArray handle the delivered array inserting its info at the linked list
		nf_justDeliveredArrayInfo.managedTypeName = NULL; //To be set by the requester function
		nf_justDeliveredArrayInfo.id = id;
		nf_justDeliveredArrayInfo.pArray = pArray;
		nf_justDeliveredArrayInfo.size = size;
	}

} //Internals

//check declaration for comments
void ReleaseManagedArray(void* pArray)
{
	ASSERT(Internals::nf_ReleaseManagedArray);
	Internals::ArrayInfo* pArrayInfo = Internals::nf_pArrayInfoLinkedListHead;
	Internals::ArrayInfo** ppArrayInfo = &Internals::nf_pArrayInfoLinkedListHead;

	while (pArrayInfo != NULL)
	{
		if (pArrayInfo->pArray == pArray)
		{
			Internals::nf_ReleaseManagedArray(pArrayInfo->id);
			*ppArrayInfo = pArrayInfo->pNext;
			delete pArrayInfo;
			return;
		}
		ppArrayInfo = &(pArrayInfo->pNext);
		pArrayInfo = pArrayInfo->pNext;
	}

	ASSERT(false); //array not found, likely a duplicated release
}

void ReleaseAllManagedArrays()
{
	while (Internals::nf_pArrayInfoLinkedListHead != NULL)
		ReleaseManagedArray(Internals::nf_pArrayInfoLinkedListHead->pArray);
}

//check declaration for comments
void* ReadFileContentToManagedArray(const char* fullFilePath, int* pSizeOutput)
{
	ASSERT(Internals::nf_RequestFileContent);
	Internals::nf_RequestFileContent(fullFilePath);
	Internals::ArrayInfo* pArrayInfo = Internals::AcquireDeliveredArray();
	if (pArrayInfo == NULL)
		return NULL;

	pArrayInfo->managedTypeName = Internals::GetManagedTypeName<void>();
	if (pSizeOutput != NULL)
		*pSizeOutput = pArrayInfo->size;

	return pArrayInfo->pArray;
}

//check declaration for comments
void SaveTextFile(const char* fullFilePath, const char* contentStr)
{
	ASSERT(Internals::nf_SaveTextFile);
	Internals::nf_SaveTextFile(fullFilePath, contentStr);
}

//check declaration for comments
void DeleteFile(const char* fullFilePath)
{
	ASSERT(Internals::nf_SaveTextFile);
	Internals::nf_SaveTextFile(fullFilePath, "DELETE");
}

//check declaration for comments
void OutputDebugStr(const char* strToLog)
{
	assert(Internals::nf_OutputDebugStr);
	Internals::nf_OutputDebugStr(strToLog);
}

//check declaration for comments
void OnAssertionFailed(const char* sourceFileName, int sourceLineNumber)
{
	if (Internals::nf_OutputDebugStr)
	{
		char assertionMsg[128];
		assertionMsg[127] = 0;
		snprintf(assertionMsg, 127, "ASSERTION FAILED: file %s, line %d", sourceFileName, sourceLineNumber);
		OutputDebugStr(assertionMsg);
	}
	assert(false);
}

} //UnityAdapter namespace
} // UnityForCpp namespace