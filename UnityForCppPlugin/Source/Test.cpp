//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

#include "Test.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "UnityAdapter.h"

namespace UnityAdapter = UnityForCpp::UnityAdapter;

//random float value from 0f to 1f
#define RANDOM_01 (((float)rand()) / (float)RAND_MAX)

//random float value from -1f to 1f.
#define RANDOM_NEG_POS_1 (2.0f*RANDOM_01 - 1.0f)

//Defines support for UnityArray<T> with Vec2 as the type T. To discover the C# type name we used "typeof(Vec2).Name" on the C# code.
UA_SUPPORTED_TYPE(UnityForCppTest::Vec2, "UnityForCppTest+Vec2");

//Number of game objects we will access not only through UnityForCpp, but also through UnityMessager directly by assigning them
//individual receiver ids which can be wrapped by GameObjectAdapter that provides exposed methods from its components to the C++ code.
//This duplication of accessing ways to the game objects DOESN'T MAKE MUCH SENSE HERE AND IT IS JUST FOR TESTING messages to components.
#define NUMBER_OF_GAME_OBJECT_ADAPTER_USAGES 10

//------------------ 

//Component declarations SHOULD BE on header files, but this is just a silly non-sense one, just for having one message to component test case
UM_DECLARE_COMPONENT(ReceiverComponentTest);
UM_DECLARE_COMPONENT_AS(UnityForCppTest, UnityForCppTestComp); 

//This class is just a "as simple as possible way" of testing sending a message to a component of receiver game object
//having the SendMessage method wrapped as a good practice. IT SHOULD NOT BE TAKEN AS A GREAT MODEL OF WRAPPING COMPONENT METHODS 
class GameObjectAdapter
{
public:
	typedef UnityForCppTest::Vec2 Vec2;

	GameObjectAdapter(int receiverId)
		: m_receiverId(receiverId) {}

	inline void SetScale(Vec2 scale)
	{
		UNITY_MESSAGER.SendMessage<ReceiverComponentTest>(m_receiverId, 0, scale);
	}

private:
	int m_receiverId;
}; // GameObjectAdapter class ----------------------------------------------------


UnityForCppTest* UnityForCppTest::s_pInstance = NULL;

void UnityForCppTest::CreateInstance(int testReceiverId, int nOfGameObjects, float randomUpdatesInterval)
{
	ASSERT(s_pInstance == NULL);
	s_pInstance = new UnityForCppTest(testReceiverId, nOfGameObjects, randomUpdatesInterval);
}

UnityForCppTest::~UnityForCppTest()
{
	//Release the positions shared array
	m_gameObjectPositions.Release();
	m_gameObjectVelocities.clear();
}

UnityForCppTest::UnityForCppTest(int testReceiverId, int nOfGameObjects, float randomUpdatesInterval)
	: m_receiverId(testReceiverId), m_numberOfGameObjects(nOfGameObjects), m_gameObjectPositions(),
	m_gameObjectVelocities(), m_timeSinceStart(0.0f), m_timeOfLastRandomUpdates(0.0f),
	m_timeOfLastLogRelatedMessage(0.0f), m_randomUpdatesInterval(randomUpdatesInterval), 
	m_receiverIdForReflectionTest(-1)
{
	TestFileRelatedFeatures();

	srand((uint32)time(NULL));

	m_gameObjectPositions.Alloc(nOfGameObjects);
	m_gameObjectVelocities.resize(nOfGameObjects);

	SetPositionsArray(m_gameObjectPositions.GetId());

	//Set random positions and velocities to the game objects, also ordering their creation at the C# side. 
	//We consider the position from -1f to 1f, being these mapped to the left most and right most screen coordinates
	for (int i = 0; i < nOfGameObjects; ++i)
	{
		//We will also access the first object created by their "ReceiverComponentTest" component, using the GameObjectAdapter below
		int receiverId = i < NUMBER_OF_GAME_OBJECT_ADAPTER_USAGES ? UNITY_MESSAGER.NewReceiverId() : -1;

		Vec2 randomPos = Vec2(RANDOM_NEG_POS_1, RANDOM_NEG_POS_1);
		m_gameObjectPositions[i] = randomPos;
		m_gameObjectVelocities[i] = Vec2(0.01f*RANDOM_NEG_POS_1, 0.01f*RANDOM_NEG_POS_1);

		InstanceGameObject(i, receiverId);

		//Access the first object created directly through UnityMessager, observe we do this after InstanceGameObject
		//otherwise the instance would not have been created yet when the resulting message of this call arrives
		if (i < NUMBER_OF_GAME_OBJECT_ADAPTER_USAGES)
		{
			if (m_receiverIdForReflectionTest < 0) //pick just one of the receiver ids for a simple test of reflection based messages
				m_receiverIdForReflectionTest = receiverId;

			GameObjectAdapter(receiverId).SetScale(Vec2(3.0f + 6.0f * RANDOM_01, 3.0f + 6.0f*RANDOM_01));
		}
	}
}

void UnityForCppTest::Update(float deltaTime)
{
	//-------1. First part of the update code: update the position for each game object, no need to send messages
	//----------since the positions array is a shared array accessed directly from the C# side. 

	//auto checkAndSolveWallCollision = [](float& pos, float dir)->bool //NOT COMPILING FOR UNITY WEBGL
	class Lambda { public:
		//the pos value for a given coordinate must not be less than -1 (for left, down directions) or more than 1 (for right,
		//up directions). When this happens we have a collision, returning true and flipping the exceeding position value.
		static inline bool checkAndSolveWallCollision(float& pos, float dir)
		{
			if (pos*dir > 1.0f)
			{
				pos = dir*(2.0f - pos*dir);
				return true;
			}

			return false;
		}; };

	for (int i = 0; i < m_numberOfGameObjects; ++i)
	{
		Vec2 newPos = m_gameObjectPositions[i] + m_gameObjectVelocities[i];

		//Check collisions for the left and right directions, flipping the velocity component when they happen
		if (Lambda::checkAndSolveWallCollision(newPos.x, -1.0f)
			|| Lambda::checkAndSolveWallCollision(newPos.x, 1.0f))
			m_gameObjectVelocities[i].x = -m_gameObjectVelocities[i].x;

		//Check collisions for the up and down  directions, flipping the velocity component when they happen
		if (Lambda::checkAndSolveWallCollision(newPos.y, -1.0f)
			|| Lambda::checkAndSolveWallCollision(newPos.y, 1.0f))
			m_gameObjectVelocities[i].y = -m_gameObjectVelocities[i].y;

		m_gameObjectPositions[i] = newPos; //updates the position on the shared array, so the C# code can get it from here
	}


	//-------2. Second part of the update code: at each given interval perform several random operation on the game objects
	//---------- testing the UnityMessager class

	m_timeSinceStart += deltaTime;
	if (m_timeSinceStart - m_timeOfLastRandomUpdates > (double)m_randomUpdatesInterval)
	{
		m_timeOfLastRandomUpdates = m_timeSinceStart;
		for (int i = 0; i < m_numberOfGameObjects; ++i)
		{
			float randomValue = RANDOM_01;
			if (randomValue < 0.33)
				SetGameObjectRotation(i, RANDOM_NEG_POS_1*180.0f);
			else if (randomValue < 0.66)
			{
				if (rand() % 2 == 0)
					SetGameObjectColor(i, RANDOM_01, RANDOM_01, RANDOM_01);
				else
				{
					float color[3] = { RANDOM_01, RANDOM_01, RANDOM_01 };
					SetGameObjectColor(i, color);
				}
			} //ELSE WE DO NOTHING FOR 1/3 OF THE GAME OBJECTS
		}
	}

	//-------3. Third part of the update code: at each given interval send random messages that result in "log to
	//-------- the Unity console" operations, which compromises too much the performance to happen oftenly. 
	//-------- These random messages we are sending here perform more complex tests on parameter packing/unpacking to messages

	if (m_timeSinceStart - m_timeOfLastLogRelatedMessage > 1.0) //interval of 1 second hard coded here
	{
		m_timeOfLastLogRelatedMessage = m_timeSinceStart;
		switch (rand() % 7)
		{
		case 0:
			UNITY_MESSAGER.SendMessage(m_receiverId, TRM_LOG_PARAM_TYPES, 8u, 37873218932819823232.3232, 3ll, "Hey", 1.2f);
			break;
		case 1:
		{
			uint64 testArray[9] = { 829, 89873929992311, 232, 32322, 23, 87, 1, 2, 3 };
			UNITY_MESSAGER.SendMessage(m_receiverId, TRM_LOG_PARAM_TYPES, UM_ARRAY_PARAM(testArray, 9), "string", 3);
			break;
		}
		case 2:
		{
			auto arrayToFill = UM_CREATE_ARRAY_TO_FILL_PARAM(int, 10);
			UNITY_MESSAGER.SendMessage(m_receiverId, TRM_LOG_PARAM_TYPES, 2.3f, "arrayToFill Test", arrayToFill, 23, "hi");
			for (int i = 0; i < 10; ++i)
				arrayToFill[i] = i;
			break;
		}
		case 3:
			DebugLogMessage("Just test sending an string message!!");
			break;
		case 4:
		{
			//Test a reflection based message on a component of an object find by "GameObject.Find"
			int iArray[5] = { 1, 2, 5, 9, 2 };
			UNITY_MESSAGER.SendMessage<UnityForCppTestComp>("UnityForCppTest", "TestReflectionBasedMessage",
																7, 0.2f, UM_ARRAY_PARAM(iArray, 5), "String Value", 1.1);
			break;
		}
		case 5:
		{
			//Test sending a message to a game object component by locating the object with "GameObject.Find", instead of using a receiverId
			float fArray[3] = { 1.2f, 3.1f, 9.2f };
			UNITY_MESSAGER.SendMessage<UnityForCppTestComp>("UnityForCppTest", TRM_LOG_PARAM_TYPES,
																		"String param", UM_ARRAY_PARAM(fArray, 3), 5);
			break;
		}
		case 6:
			if (m_receiverIdForReflectionTest >= 0)
			{
				//Test sending a reflection based message to a component from a game object we have instanced
				uint8 bArray[5] = { 1, 2, 5, 9, 2 };
				UNITY_MESSAGER.SendMessage<ReceiverComponentTest>(4, "TestReflectionBasedMessage", 
																		5, 9.1f, UM_ARRAY_PARAM(bArray, 5), "Samuel");
			}
			break;
		}
	}
}

void UnityForCppTest::TestFileRelatedFeatures()
{
	DEBUG_LOG("Requesting the FileTest.txt from C++");
	UnityArray<uint8> fileContent;
	UnityAdapter::ReadFileContentToUnityArray("FileTest.txt", &fileContent);
	ASSERT(fileContent.GetPtr());

	DEBUG_LOG((const char*)fileContent.GetPtr());

	DEBUG_LOG("Saving the file TestFolder1/TestFolder2/FileSavingTest.txt from C++");
	UnityAdapter::SaveTextFile("TestFolder1/TestFolder2/FileSavingTest.txt", "This was saved to a file from an UnityPlugin");

	DEBUG_LOG("Loading the saved file and printing its content");
	UnityArray<uint8> savedFileContent; 
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
}
