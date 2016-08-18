//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

//THIS SOURCE FILE HAS THREE PARTS:
//
//1. UnityForCppTest class header 
//2. UnityForCppTest Plugin interface (the functions exposed on the DLL)
//3. UnityForCppTest class implementation
//
//We keep them all together in the same file in order to avoid having 3 test related source files

//++++++++++++ PART 1: UnityForCpppTest class header ++++++++++++++++++++++++++++++++

#include "Shared.h"
#include "UnityArray.h"
#include "UnityMessager.h"
#include <vector>

using UnityForCpp::UnityArray;
using std::vector;

class UnityForCppTest
{
public:
	//Singleton access point
	static UnityForCppTest& GetInstance() { ASSERT(s_pInstance); return *s_pInstance; }

	//Each time the game is started via the editor this method restart the test case to its initial state
	//Also it include the tests for the file read/writing features
	static void CreateInstance(int testReceiverId, int nOfGameObjects, float randomUpdatesInterval);

	//Delete the singleton class instance, important on the OnDestroy method in order to release the shared arrays in use.
	static void DeleteInstance() { DELETE(s_pInstance); }

	//Updates the test case, simulating a game update, checks the implementation for details about what it does.
	void Update(float deltaTime);

	//Custom type to be tested when used with UnityArray. Observe it is composed by blittable types,
	//has a corresponding declarion at the C# side and instanced as supported type using the macro
	//UA_SUPPORTED_TYPE at the implementation part of this class.
	typedef struct Vec2
	{
		float x;
		float y;

		Vec2(float _x, float _y) : x(_x), y(_y) {}
		Vec2() : x(0.0f), y(0.0f) {}

		Vec2 operator+(Vec2& v) { return Vec2(x + v.x, y + v.y); }
		Vec2 operator-(Vec2& v) { return Vec2(x - v.x, y - v.y); }
		Vec2 operator*(float& f) { return Vec2(x*f, y*f); }
	} Vec2;

private:
	UnityForCppTest(int testReceiverId, int nOfGameObjects, float randomUpdatesInterval);
	~UnityForCppTest();
	
	//we isolate file related test operations on this method, since the rest of the test doesn't use files at all.
	void TestFileRelatedFeatures();

	int m_testReceiverId; //receiverId allocated to the UnityForCppTest singleton receiver from the C# and passed on the initialization
	int m_numberOfGameObjects; //number of game objects to order the creation and manage from the C++ code, passed on the initialization
	UnityArray<Vec2> m_gameObjectPositions; //shared array containing the position of each managed game object.
	vector<Vec2> m_gameObjectVelocities; //vector containing the velocity of each managed game object.
	double m_timeSinceStart;
	double m_timeOfLastRandomUpdates;
	double m_timeOfLastLogRelatedMessage; //so we avoid sending messages the result in logging calls all the time, what would compromise the performance
	float m_randomUpdatesInterval; //passed on initialization, positions are always updated, orther stuff are randomly updated after this interval

	static UnityForCppTest* s_pInstance;

	//------------ START OF THE EXPOSED C# INTERFACE -------------------------
	enum TestReceiverMessages
	{
		TRM_SET_GAME_OBJECT_ROTATION = 0,
		TRM_SET_GAME_OBJECT_SCALE = 1, 
		TRM_SET_GAME_OBJECT_COLOR = 2,
		TRM_SET_POSITIONS_ARRAY = 3,
		TRM_INSTANCE_GAME_OBJECT = 4,
		TRM_DEBUG_LOG_MESSAGE = 5,
		TRM_LOG_PARAM_TYPES = 6 //([ANY, ...]) => not wrapped on a method call, used directly with UNITY_MESSAGER.SendMessage.
	};

	inline void SetGameObjectRotation(int gameObjectId, float rotation)
	{
		UNITY_MESSAGER.SendMessage(m_testReceiverId, TRM_SET_GAME_OBJECT_ROTATION, gameObjectId, rotation);
	}

	inline void SetGameObjectScale(int gameObjectId, Vec2 scale)
	{
		UNITY_MESSAGER.SendMessage(m_testReceiverId, TRM_SET_GAME_OBJECT_SCALE, gameObjectId, scale);
	}

	inline void SetGameObjectColor(int gameObjectId, float r, float g, float b)
	{
		UNITY_MESSAGER.SendMessage(m_testReceiverId, TRM_SET_GAME_OBJECT_COLOR, gameObjectId, r, g, b);
	}

	inline void SetGameObjectColor(int gameObjectId, float colorComponents[3])
	{
		UNITY_MESSAGER.SendMessage(m_testReceiverId, TRM_SET_GAME_OBJECT_COLOR, gameObjectId, UM_ARRAY_PARAM(colorComponents, 3));
	}

	inline void SetPositionsArray(int arrayId)
	{
		UNITY_MESSAGER.SendMessage(m_testReceiverId, TRM_SET_POSITIONS_ARRAY, arrayId);
	}

	inline void InstanceGameObject(int gameObjectId)
	{
		UNITY_MESSAGER.SendMessage(m_testReceiverId, TRM_INSTANCE_GAME_OBJECT, gameObjectId);
	}

	inline void DebugLogMessage(const char* strToLog, int logType = 0)
	{
		UNITY_MESSAGER.SendMessage(m_testReceiverId, TRM_DEBUG_LOG_MESSAGE, strToLog, logType);
	}

	//------------ END OF THE EXPOSED C# INTERFACE -------------------------

}; //UnityForCppTest class


//++++++++++++ PART 2: UnityForCpppTest Plugin interface (methods exposed by the DLL) +++++++++++++++++++

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

//++++++++++++ PART 2: UnityForCpppTest implementation ++++++++++++++++++++++++++++++++++++++++++++++++++

#include <stdio.h>
#include <stdlib.h>
#include "UnityAdapter.h"
#include <time.h>

namespace UnityAdapter = UnityForCpp::UnityAdapter;

//random float value from 0f to 1f
#define RANDOM_01 (((float)rand()) / (float)RAND_MAX)

//random float value from -1f to 1f.
#define RANDOM_NEG_POS_1 (2.0f*RANDOM_01 - 1.0f)

//Defines support for UnityArray<T> with Vec2 as the type T. To discover the C# type name we used "typeof(Vec2).Name" on the C# code.
UA_SUPPORTED_TYPE(UnityForCppTest::Vec2, "UnityForCppTest+Vec2");

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
	: m_testReceiverId(testReceiverId), m_numberOfGameObjects(nOfGameObjects), m_gameObjectPositions(),
	m_gameObjectVelocities(), m_timeSinceStart(0.0f), m_timeOfLastRandomUpdates(0.0f),
	m_timeOfLastLogRelatedMessage(0.0f), m_randomUpdatesInterval(randomUpdatesInterval)
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
		Vec2 randomPos = Vec2(RANDOM_NEG_POS_1, RANDOM_NEG_POS_1);
		m_gameObjectPositions[i] = randomPos;
		m_gameObjectVelocities[i] = Vec2(0.01f*RANDOM_NEG_POS_1, 0.01f*RANDOM_NEG_POS_1);
		InstanceGameObject(i);
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
				SetGameObjectScale(i, Vec2(1.0f + 2 * RANDOM_01, 1.0f + 2.0f*RANDOM_01));
			else if (randomValue < 0.99)
			{
				if (rand() % 2 == 0)
					SetGameObjectColor(i, RANDOM_01, RANDOM_01, RANDOM_01);
				else
				{
					float color[3] = { RANDOM_01, RANDOM_01, RANDOM_01 };
					SetGameObjectColor(i, color);
				}
			}
		}
	}

	//-------2. Third part of the update code: at each given interval send random messages that result in "log to
	//---------- the Unity console" operations, which compromises too much the performance to happen oftenly. 
	//-------- These random messages we are sending here perform more complex tests on parameter packing/unpacking to messages

	if (m_timeSinceStart - m_timeOfLastLogRelatedMessage > 1.0) //interval of 1 second hard coded here
	{
		m_timeOfLastLogRelatedMessage = m_timeSinceStart;
		switch (rand() % 4)
		{
		case 0:
			UNITY_MESSAGER.SendMessage(m_testReceiverId, TRM_LOG_PARAM_TYPES, 8u, 37873218932819823232.3232, 3ll, "Hey", 1.2f);
			break;
		case 1:
		{
			uint64 testArray[9] = { 829, 89873929992311, 232, 32322, 23, 87, 1, 2, 3 };
			UNITY_MESSAGER.SendMessage(m_testReceiverId, TRM_LOG_PARAM_TYPES, UM_ARRAY_PARAM(testArray, 9), "string", 3);
			break;
		}
		case 2:
		{
			auto arrayToFill = UM_CREATE_ARRAY_TO_FILL_PARAM(int, 10);
			UNITY_MESSAGER.SendMessage(m_testReceiverId, TRM_LOG_PARAM_TYPES, 2.3f, "arrayToFill Test", arrayToFill, 23, "hi");
			for (int i = 0; i < 10; ++i)
				arrayToFill[i] = i;
			break;
		}
		case 3:
			DebugLogMessage("Just test sending an string message!!");
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