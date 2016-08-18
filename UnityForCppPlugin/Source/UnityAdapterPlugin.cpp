//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

#include "Shared.h"
#include "UnityAdapter.h"

namespace UAInternals = UnityForCpp::UnityAdapter::Internals;

//Unity Adapter DLL interface, used from C# to setup the unity feature provided to the C++ code
extern "C"
{
	//Sets the function pointer for the C# function providing the Debug.Log feature
	//UnityAdapter.OutputDebugStr (C#) is the expected function check it for more comments 
	void EXPORT_API UA_SetOutputDebugStrFcPtr(UAInternals::OutputDebugStrFcPtr fcPtr)
	{
		UAInternals::SetOutputDebugStrFcPtr(fcPtr);
	}

	//Sets the function pointers for the C# functions providing the file reading and saving (including delete)
	//UnityAdapter.RequestFileContent and UnityAdapter.SaveTextFile (both C#) are expected, check their comments
	void EXPORT_API UA_SetFileFcPtrs(UAInternals::RequestFileContentFcPtr requestFileContentFcPtr,
									 UAInternals::SaveTextFileFcPtr openAndWriteAllFileFcPtr)
	{
		UAInternals::SetFileFcPtrs(requestFileContentFcPtr, openAndWriteAllFileFcPtr);
	}

	//Sets the function pointers for the C# functions providing shared arrays from the managed memory
	//UnityAdapter.RequestManagedArray and UnityAdapter.ReleaseManagedArray (both C#) are expected, check their comments
	void EXPORT_API UA_SetArrayFcPtrs(UAInternals::RequestManagedArrayFcPtr requestManagedArrayFcPtr,
									  UAInternals::ReleaseManagedArrayFcPtr releaseManagedArrayFcPtr)
	{
		UAInternals::SetArrayFcPtrs(requestManagedArrayFcPtr, releaseManagedArrayFcPtr);
	}

	//Function used by the C# code as the way to return the requested managed array by UAInternals::NewManagedArray 
	//and also to deliver the file content requested via UAInternals::ReadFileContentToManagedArray
	void EXPORT_API UA_DeliverManagedArray(int id, void* pArray, int length)
	{
		UAInternals::DeliverRequestedManagedArray(id, pArray, length);
	}
} // end of export C block
