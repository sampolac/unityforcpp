//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#include "Shared.h"
#include "UnityAdapter.h"
#include <stdio.h>

namespace UnityAdapter = UnityForCpp::UnityAdapter;

//Short code for testing the currently available UnityForCpp Features
extern "C"
{
	//TEST_ARRAY_SIZE must be bigger than 4, since we print the first 4 items at T_PrintSharedFloatArrayFromCpp
#define TEST_ARRAY_SIZE 4

	//We will change this int array from C++ and print the result from C#
	static int f_sharedIntArrayId = -1;
	static int* f_sharedIntArray = NULL;

	//We will change this float array from C# and print the result from C++
	static int f_sharedFloatArrayId = -1;
	static float* f_sharedFloatArray = NULL;	

	void EXPORT_API T_InitTest()
	{
		//TESTING FILE READING/WRITING FEATURES ----------------------------------

		DEBUG_LOG("Requesting the FileTest.txt from C++");
		int size;
		char* fileContent = (char*) UnityAdapter::ReadFileContentToManagedArray("FileTest.txt", &size);
		DEBUG_LOG(fileContent);

		DEBUG_LOG("Saving the file TestFolder1/TestFolder2/FileSavingTest.txt from C++");
		UnityAdapter::SaveTextFile("TestFolder1/TestFolder2/FileSavingTest.txt", "This was saved to a file from an UnityPlugin");

		DEBUG_LOG("Loading the saved file and printing its content");
		char* savedFileContent = (char*)UnityAdapter::ReadFileContentToManagedArray("TestFolder1/TestFolder2/FileSavingTest.txt", &size);
		DEBUG_LOG(savedFileContent);

		DEBUG_LOG("Deleting the file FileSavingTest.txt from C++");
		UnityAdapter::DeleteFile("TestFolder1/TestFolder2/FileSavingTest.txt");

		//release both file contents we have loaded
		UnityAdapter::ReleaseManagedArray(fileContent);
		UnityAdapter::ReleaseManagedArray(savedFileContent);

		//TESTING SHARED ARRAYS		
		//Since the arrays are requested and released from a C++ code "decision", it is like C++ owns the arrays, so we
		//initialize their values here, even for the float array that is mostly intended to be written from the C# code

		DEBUG_LOG("Requesting shared arrays from C++ code");

		f_sharedIntArrayId = UnityAdapter::NewManagedArray<int>(TEST_ARRAY_SIZE);
		f_sharedIntArray = UnityAdapter::GetManagedArray<int>(f_sharedIntArrayId);

		for (int i = 0; i < TEST_ARRAY_SIZE; ++i)
			f_sharedIntArray[i] = 0;

		f_sharedFloatArrayId = UnityAdapter::NewManagedArray<float>(TEST_ARRAY_SIZE);
		f_sharedFloatArray = UnityAdapter::GetManagedArray<float>(f_sharedFloatArrayId);
		
		for (int i = 0; i < TEST_ARRAY_SIZE; ++i)
			f_sharedFloatArray[i] = 0.0f;

		DEBUG_LOG("The shared arrays have been delivered and initialized with success");
	}

	int EXPORT_API T_GetSharedIntArrayId()
	{
		return f_sharedIntArrayId;
	}

	int EXPORT_API T_GetSharedFloatArrayId()
	{
		return f_sharedFloatArrayId;
	}

	void EXPORT_API T_ChangeSharedIntArray()
	{
		for (int i = 0; i < TEST_ARRAY_SIZE; ++i)
			f_sharedIntArray[i] += 1;
	}

	void EXPORT_API T_PrintSharedFloatArray()
	{
		//actually we will print just the 4 first items of the array
		char outputStr[256];
		outputStr[255] = 0; //make sure it will be zero terminated
		snprintf(outputStr, 255, 
			     "First 4 values of the float array being printed from C++: %.2f %.2f %.2f %.2f", 
				 f_sharedFloatArray[0], f_sharedFloatArray[1], f_sharedFloatArray[2], f_sharedFloatArray[3]);

		DEBUG_LOG(outputStr);
	}

	void EXPORT_API T_TerminateTest()
	{
		//Set values back to their initial values, since the Unity Editor may restart the test execution
		f_sharedIntArrayId = -1;
		f_sharedIntArray = NULL;

		f_sharedFloatArrayId = -1;
		f_sharedFloatArray = NULL;
		
		//Release the shared arrays
		UnityAdapter::ReleaseAllManagedArrays();
	}
} // end of export C block
