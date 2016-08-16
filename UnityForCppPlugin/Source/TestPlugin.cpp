//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#include "Shared.h"
#include "UnityAdapter.h"
#include "UnityArray.h"
#include <stdio.h>

namespace UnityAdapter = UnityForCpp::UnityAdapter;
using UnityForCpp::UnityArray;

namespace TestPlugin
{
	//We will change this int array from C++ and print the result from C#
	static UnityArray<int> nf_intUnityArray;

	//We will change this float array from C# and print the result from C++
	static UnityArray<float> nf_floatUnityArray;
}

//Short code for testing the currently available UnityForCpp Features
extern "C"
{
	//TEST_ARRAY_SIZE must be bigger than 4, since we print the first 4 items at T_PrintSharedFloatArrayFromCpp
#define TEST_ARRAY_SIZE 4
	
	void EXPORT_API T_InitTest()
	{
		//TESTING FILE READING/WRITING FEATURES ----------------------------------

		DEBUG_LOG("Requesting the FileTest.txt from C++");
		UnityArray<uint8> fileContent; //No need to call Alloc, ReadFileContentToUnityArray allocates it
		UnityAdapter::ReadFileContentToUnityArray("FileTest.txt", &fileContent);
		ASSERT(fileContent.GetPtr());

		DEBUG_LOG((const char*)fileContent.GetPtr());

		DEBUG_LOG("Saving the file TestFolder1/TestFolder2/FileSavingTest.txt from C++");
		UnityAdapter::SaveTextFile("TestFolder1/TestFolder2/FileSavingTest.txt", "This was saved to a file from an UnityPlugin");

		DEBUG_LOG("Loading the saved file and printing its content");
		UnityArray<uint8> savedFileContent; //No need to call Alloc, ReadFileContentToUnityArray allocates it
		UnityAdapter::ReadFileContentToUnityArray("TestFolder1/TestFolder2/FileSavingTest.txt", &savedFileContent);
		ASSERT(savedFileContent.GetPtr());

		DEBUG_LOG((const char*)savedFileContent.GetPtr());

		DEBUG_LOG("Deleting the file FileSavingTest.txt from C++");
		UnityAdapter::DeleteFile("TestFolder1/TestFolder2/FileSavingTest.txt");

		//test the file was successfully deleted above
		savedFileContent.Release();
		UnityAdapter::ReadFileContentToUnityArray("TestFolder1/TestFolder2/FileSavingTest.txt", &savedFileContent);
		ASSERT(savedFileContent.GetPtr() == NULL);
		DEBUG_LOG("We have successfully deleted the file FileSavingTest.txt!");

		//TESTING SHARED ARRAYS		
		//Since the arrays are requested and released from a C++ code "decision", it is like C++ owns the arrays, so we
		//initialize their values here, even for the float array that is mostly intended to be written from the C# code

		DEBUG_LOG("Requesting shared arrays from C++ code");
		TestPlugin::nf_intUnityArray.Alloc(TEST_ARRAY_SIZE);
		for (int i = 0; i < TEST_ARRAY_SIZE; ++i)
			TestPlugin::nf_intUnityArray[i] = 0;

		TestPlugin::nf_floatUnityArray.Alloc(TEST_ARRAY_SIZE);
		for (int i = 0; i < TEST_ARRAY_SIZE; ++i)
			TestPlugin::nf_floatUnityArray[i] = 0;
		
		DEBUG_LOG("The shared arrays have been delivered and initialized with success");
	}

	int EXPORT_API T_GetSharedIntArrayId()
	{
		return TestPlugin::nf_intUnityArray.GetId();
	}

	int EXPORT_API T_GetSharedFloatArrayId()
	{
		return TestPlugin::nf_floatUnityArray.GetId();
	}

	void EXPORT_API T_ChangeSharedIntArray()
	{
		for (int i = 0; i < TEST_ARRAY_SIZE; ++i)
			TestPlugin::nf_intUnityArray[i] += 1;
	}

	void EXPORT_API T_PrintSharedFloatArray()
	{
		//actually we will print just the 4 first items of the array
		char outputStr[256];
		outputStr[255] = 0; //make sure it will be zero terminated
		snprintf(outputStr, 255, 
			     "First 4 values of the float array being printed from C++: %.2f %.2f %.2f %.2f", 
				 TestPlugin::nf_floatUnityArray[0], TestPlugin::nf_floatUnityArray[1], 
				 TestPlugin::nf_floatUnityArray[2], TestPlugin::nf_floatUnityArray[3]);

		DEBUG_LOG(outputStr);
	}

	void EXPORT_API T_TerminateTest()
	{
		TestPlugin::nf_intUnityArray.Release();
		TestPlugin::nf_floatUnityArray.Release();
	}
} // end of export C block
