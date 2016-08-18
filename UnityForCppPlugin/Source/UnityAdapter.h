//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#ifndef UNITY_ADAPTER_H
#define UNITY_ADAPTER_H

#include "Shared.h"


namespace UnityForCpp
{

template <typename T> class UnityArray;

//Provides unity multiplatform features to the C++ code
//Works associated with UnityAdapter.cs at the C# side using UnityAdapterPlugin.cpp for interop
//WARNING: Your cpp runtime state will persist across multiple game replays from a same Unity Editor usage 
namespace UnityAdapter
{
// -------------------------- UNITY ADAPTER UTILITIES FOR THE CPP CODE ----------------------------

// File utilities ----------------------------------

//Open the file as TextAsset and read its content to an unity array. Returns true in case of success and false if
//the file could not be found. DO NOT CALL "Alloc" on the output array, this is done by the method. 
//fullFilePath may (and should) include the file extension.
//The search *goes first* for existing saved files and after for bundle files. Saved files will have the persistent
//data folder as path root. Bundle files are expected to have an Assets "Resources" folder as path root.
bool ReadFileContentToUnityArray(const char* fullFilePath, UnityArray<uint8>* pUnityArrayOutput);

//Save the contentStr data to the file at the specified path (creates file and directory if needed).
//The persistent data folder for the platform will be the path root. An existing file for this path will be overwriten.
void SaveTextFile(const char* fullFilePath, const char* contentStr);

//Deletes the file at the specified path if it exists. The persistent data folder for the platform will be the path root. 
void DeleteFile(const char* fullFilePath);

// Debug utilities ----------------------------------

//These defined int values are expected by UnityAdapter.OutputDebugStr at the C# code 
#define UA_NORMAL_LOG 0
#define UA_WARNING_LOG 1
#define UA_ERROR_LOG 2

//Check comments for the DEBUG_LOG, WARNING_LOG, ERROR_LOG macros, use them instead
//logType == UA_NORMAL_LOG (uses Debug.Log), logType == UA_WARNING_LOG (uses Debug.LogWarning), logType == UA_ERROR_LOG (uses Debug.LogError)
void OutputDebugStr(int logType, const char* strToLog);

// Shared memory utilities -----------------

//USES UnityArray<TYPE> INSTEAD. Only use this method directly if you have a very special reason.
//This method request a new shared/managed (C#) array, having the given "length" and type specified by its .NET type.
//Returns the arrayId for received array, which can be provided to the C# code as an way for this to use the array
//For checking the possible array types, check the usage of the macro UA_SUPPORTED_TYPE on UnityArray.h
//This method should never fail, if it does a C assertion will be triggered.
int NewManagedArray(const char* managedTypeName, int length, void** pOutputArrayPtr);

//Release the shared/managed C# array
void ReleaseManagedArray(int arrayId);


//namespace to separate actual utilities offered by UnityAdapter to the cpp code from the internal namespace stuff
//providing this implementation and that needs to be accessible to UnityAdapterPlugin.h
namespace Internals
{ 
	struct DeliveredManagedArray;

	// ---------------------- Interface for C++/C# communication through the UnityAdapterPlugin -------------------

	//At C# UnityForCpp.UnityAdapter.UnityAdapterDLL defines delegates for each one of these function pointer types
	typedef void(*OutputDebugStrFcPtr)(int, const char *); //(logType, string) -> string to pass to Debug.Log()
	typedef void(*RequestFileContentFcPtr)(const char *);//(fullFilePath) -> file content should be returned via SetFileContent
	typedef void(*SaveTextFileFcPtr)(const char *, const char*); //(fullFilePath, contentAsStr) 
	typedef void(*RequestManagedArrayFcPtr)(const char*, int); //(dotNETTypeName, arrayLength)
	typedef void(*ReleaseManagedArrayFcPtr)(int); //(arrayId)

	//Check for comments at UnityAdapterPlugin.h
	void SetOutputDebugStrFcPtr(OutputDebugStrFcPtr fcPtr);

	//Check for comments at UnityAdapterPlugin.h
	void SetFileFcPtrs(RequestFileContentFcPtr requestFileContentFcPtr,
						SaveTextFileFcPtr openAndWriteAllFileFcPtr);

	//Check for comments at UnityAdapterPlugin.h
	void SetArrayFcPtrs(RequestManagedArrayFcPtr requestManagedArrayFcPtr,
						ReleaseManagedArrayFcPtr releaseManagedArrayFcPtr);

	//Check for comments at UnityAdapterPlugin.h
	void DeliverRequestedManagedArray(int id, void* pArray, int length);

} //Internals namespace

} //UnityAdapter namespace

} //UnityForCpp namespace



#endif