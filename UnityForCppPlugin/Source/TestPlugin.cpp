//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

#include "Test.h"


extern "C"
{	
	//To be called when UnityForCppTest C# instance gets Start called
	void EXPORT_API T_InitTest(int testReceiverId, int nOfGameObjects, float randomUpdatesInterval)
	{
		UnityForCppTest::CreateInstance(testReceiverId, nOfGameObjects, randomUpdatesInterval);
	}

	//To be called when UnityForCppTest C# instance gets "Update" called
	void EXPORT_API T_UpdateTest(float deltaTime)
	{
		UnityForCppTest::GetInstance().Update(deltaTime);
	}

	//To be called when UnityForCppTest C# instance gets OnDestroy called
	void EXPORT_API T_TerminateTest()
	{
		UnityForCppTest::DeleteInstance();
	}
} // end of export C block
