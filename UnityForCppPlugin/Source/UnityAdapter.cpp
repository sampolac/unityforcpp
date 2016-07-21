//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#include "UnityAdapter.h"
#include "UnityArray.h"
#include <assert.h>
#include <stdio.h>

namespace UnityForCpp
{
namespace UnityAdapter
{
namespace Internals
{
	//struct to hold the received array pointer and info, also used to access the private contructor from UnityArray<>
	struct DeliveredManagedArray
	{
		int id;
		int count;
		void* pArray;

		DeliveredManagedArray()
			: id(-1), count(0), pArray(NULL) {}

		DeliveredManagedArray(int _id, int _count, void* _pArray)
			: id(_id), count(_count), pArray(_pArray) {}

		//provides an way for UnityAdapter functions be able to receive arrays from C# and deliver them directly as UnityArray<>
		template <typename T> UnityArray<T>* GetAsNewUnityArray()
		{
			return new UnityArray<T>(id, count, pArray);
		}
	};

	//ALWAYS USE THIS WITH "pNextToGet=NULL" TO RETRIEVE THE DELIVERED ARRAY FROM UA_DeliverManagedArray DLL function
	//if the returned struct has pArray == NULL, this means the C# code could not deliver the requested array
	//This method is as it is in order to prevent possible wrong manipulations with the received array info by the user functions
	DeliveredManagedArray GetDeliveredManagedArrayAndSetNext(DeliveredManagedArray* pNextToGet)
	{
		static DeliveredManagedArray c_deliveredManagedArray = DeliveredManagedArray();

		//Assert we have no forgot delivered array, previously delivered, but not properly acquired
		ASSERT(c_deliveredManagedArray.pArray == NULL || pNextToGet == NULL);

		DeliveredManagedArray returnValue = c_deliveredManagedArray;
		c_deliveredManagedArray = pNextToGet ? *pNextToGet : DeliveredManagedArray();

		return returnValue;
	}

	//----------------
	//"f" prefix stands for file variable (C static variable), "g" for global variable, "n" for namaspace global variable
	//"s" for class static attribute, "m" for class instance attribute, "c" for function static variables

	//C# function pointers to be set right after the DLL loading via the UnityAdapterPlugin interface
	static OutputDebugStrFcPtr			nf_OutputDebugStr = NULL;
	static RequestFileContentFcPtr		nf_RequestFileContent = NULL;
	static SaveTextFileFcPtr			nf_SaveTextFile = NULL;
	static RequestManagedArrayFcPtr		nf_RequestManagedArray = NULL;
	static ReleaseManagedArrayFcPtr		nf_ReleaseManagedArray = NULL;

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

	void DeliverRequestedManagedArray(int id, void* pArray, int count)
	{
		if (!pArray)
			return;

		DeliveredManagedArray deliveredArray(id, count, pArray);
		GetDeliveredManagedArrayAndSetNext(&deliveredArray);
	}

} //Internals

//check declaration for comments
int NewManagedArray(const char* managedTypeName, int count, void** pOutputArrayPtr)
{
	ASSERT(Internals::nf_RequestManagedArray);
	ASSERT(pOutputArrayPtr);

	Internals::nf_RequestManagedArray(managedTypeName, count);
	struct Internals::DeliveredManagedArray deliveredArray = Internals::GetDeliveredManagedArrayAndSetNext(NULL);

	ASSERT(deliveredArray.pArray != NULL);

	*pOutputArrayPtr = deliveredArray.pArray;
	return deliveredArray.id;
}

//check declaration for comments
void ReleaseManagedArray(int arrayId)
{
	ASSERT(Internals::nf_ReleaseManagedArray);
	Internals::nf_ReleaseManagedArray(arrayId);
}

//check declaration for comments
UnityArray<uchar>* ReadFileContentToUnityArray(const char* fullFilePath)
{
	ASSERT(Internals::nf_RequestFileContent);
	Internals::nf_RequestFileContent(fullFilePath);
	struct Internals::DeliveredManagedArray deliveredArray = Internals::GetDeliveredManagedArrayAndSetNext(NULL);

	if (deliveredArray.pArray == NULL)
		return NULL;

	return deliveredArray.GetAsNewUnityArray<uchar>();
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