//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#ifndef UNITY_ADAPTER_H
#define UNITY_ADAPTER_H

#include "Shared.h"

#ifdef _DEBUG

//Log str to Unity console using "Debug.Log"
#define DEBUG_LOG(str) UnityForCpp::UnityAdapter::OutputDebugStr(str)

//When exp is false, logs its source file name and line before triggering an usual C assertion 
#define ASSERT(exp) if (!(exp)) UnityForCpp::UnityAdapter::OnAssertionFailed(__FILE__, __LINE__)

#else //release version
#define DEBUG_LOG(str)
#define ASSERT(str)
#endif

namespace UnityForCpp
{

//Provides unity multiplatform features to the C++ code
//Works associated with UnityAdapter.cs at the C# side using UnityAdapterPlugin.cpp for interop
//WARNING: Your cpp runtime state will persist across multiple game replays from a same Unity Editor usage 
namespace UnityAdapter
{
// -------------------------- UNITY ADAPTER UTILITIES FOR THE CPP CODE ----------------------------

// Shared memory utilities --------------------------

//Gets an existing shared/managed array and (optionally) its size, if a non-null pSizeOutput is provided
//IMPORTANT: type safety check at runtime. Requesting the array with the wrong type results on a assertion
//Check NewManagedArray comments for more info about the possible types
template <typename T> T* GetManagedArray(int arrayId, int* pSizeOutput = NULL);

//Request a new shared/managed (C#) array, having the given "size" and type (<T>), to the C# code.
//Returns the arrayId for received array, use it with GetManagedArray above in order to retrive the array itself
//Also this array id allows the shared array to be found by the C# code, once you provide it to the C# code. 
//For checking/extending the possible array types, check the usage of the macro UA_SUPPORTED_TYPE on UnityAdapter.cpp
//This method should never fail, if it does a C assertion will be triggered.
template <typename T> int NewManagedArray(int size);

//Release the shared/managed C# array by providing its head pointer
//if the corresponding array info is not found an assertion will be triggered (possibly duplicated release)
void ReleaseManagedArray(void* pArray);

//Released all the managed arrays currently being held by the C++ code.
//IMPORTANT: make your logic to get aware about the execution end of a given "play" from the Unity Editor and call
//this function to be sure you are not leaving any allocated arrays between to game replays
void ReleaseAllManagedArrays();


// File utilities ----------------------------------

//Open the file as TextAsset and read its content to a managed array and returns it or returns NULL if not found.
//You must release this array with ReleaseManagedArray when you are done with it.
//fullFilePath may (and should) include the file extension.
//pSizeOutput is optional, since a text file content should ends with 0, but it's better to provided and rely on it.
//The search *goes first* for existing saved files and after for bundle files. Saved files will have the persistent
//data folder as path root. Bundle files are expected to have an Assets "Resources" folder as path root.
void* ReadFileContentToManagedArray(const char* fullFilePath, int* pSizeOutput);

//Save the contentStr data to the file at the specified path (creates file and directory if needed).
//The persistent data folder for the platform will be the path root. An existing file for this path will be overwriten.
void SaveTextFile(const char* fullFilePath, const char* contentStr);

//Deletes the file at the specified path if it exists. The persistent data folder for the platform will be the path root. 
void DeleteFile(const char* fullFilePath);


// Debug utilities ----------------------------------

//Check comments for the DEBUG_LOG macro
void OutputDebugStr(const char* strToLog);

//Check comments for the ASSERT macro
void OnAssertionFailed(const char* sourceFileName, int sourceLineNumber); //use macro instead

//namespace to separate actual utilities offered by UnityAdapter to the cpp code from the internal namespace stuff
//providing this implementation and that needs to be accessible to UnityAdapterPlugin.h
namespace Internals
{ 
	// ---------------------- Interface for C++/C# communication through the UnityAdapterPlugin -------------------

	//At C# UnityForCpp.UnityAdapter.UnityAdapterDLL defines delegates for each one of these function pointer types
	typedef void(*OutputDebugStrFcPtr)(const char *); //(string) -> string to pass to Debug.Log()
	typedef void(*RequestFileContentFcPtr)(const char *);//(fullFilePath) -> file content should be returned via SetFileContent
	typedef void(*SaveTextFileFcPtr)(const char *, const char*); //(fullFilePath, contentAsStr) 
	typedef void(*RequestManagedArrayFcPtr)(const char*, int); //(dotNETTypeName, arraySize)
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
	void DeliverRequestedManagedArray(int id, void* pArray, int size);

	//forward declaration of functions needed for defining the template functions, check UnityAdapter.cpp for comments about them
	template <typename T> const char* GetManagedTypeName();
	void* GetManagedArray(int arrayId, int* pSizeOutput, const char* managedTypeName);
	int NewManagedArray(int size, const char* managedTypeName);
} //Internals namespace


// ----------------------- Default Template functions' definition ----------------------------
template <typename T>
inline T* GetManagedArray(int arrayId, int* pSizeOutput)
{
	return reinterpret_cast<T*>(Internals::GetManagedArray(arrayId, pSizeOutput, Internals::GetManagedTypeName<T>()));
}

template <typename T>
inline int NewManagedArray(int size)
{
	return Internals::NewManagedArray(size, Internals::GetManagedTypeName<T>());
}


} //UnityAdapter namespace
} //UnityForCpp namespace

















#endif