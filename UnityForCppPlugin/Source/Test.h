//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

#ifndef TEST_H
#define TEST_H

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

	//------------ START OF THE EXPOSED C# INTERFACE -------------------------
	enum TestReceiverMessages
	{
		TRM_SET_GAME_OBJECT_ROTATION = 0,
		TRM_SET_GAME_OBJECT_COLOR = 1,
		TRM_SET_POSITIONS_ARRAY = 2,
		TRM_INSTANCE_GAME_OBJECT = 3,
		TRM_DEBUG_LOG_MESSAGE = 4,
		TRM_LOG_PARAM_TYPES = 5 //([ANY, ...]) => not wrapped on a method call, to be used internally with UNITY_MESSAGER.SendMessage
	};

	inline void SetGameObjectRotation(int gameObjectId, float rotation)
	{
		UNITY_MESSAGER.SendMessage(m_receiverId, TRM_SET_GAME_OBJECT_ROTATION, gameObjectId, rotation);
	}

	inline void SetGameObjectColor(int gameObjectId, float r, float g, float b)
	{
		UNITY_MESSAGER.SendMessage(m_receiverId, TRM_SET_GAME_OBJECT_COLOR, gameObjectId, r, g, b);
	}

	inline void SetGameObjectColor(int gameObjectId, float colorComponents[3])
	{
		UNITY_MESSAGER.SendMessage(m_receiverId, TRM_SET_GAME_OBJECT_COLOR, gameObjectId, UM_ARRAY_PARAM(colorComponents, 3));
	}

	inline void DebugLogMessage(const char* strToLog, int logType = 0)
	{
		UNITY_MESSAGER.SendMessage(m_receiverId, TRM_DEBUG_LOG_MESSAGE, strToLog, logType);
	}

private:
	inline void SetPositionsArray(int arrayId)
	{
		UNITY_MESSAGER.SendMessage(m_receiverId, TRM_SET_POSITIONS_ARRAY, arrayId);
	}

	inline void InstanceGameObject(int gameObjectId, int receiverId = -1)
	{
		UNITY_MESSAGER.SendMessage(m_receiverId, TRM_INSTANCE_GAME_OBJECT, gameObjectId, receiverId);
	}

	//------------ END OF THE EXPOSED C# INTERFACE -------------------------
public:
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

	int m_receiverId; //receiverId allocated to the UnityForCppTest singleton receiver from the C# and passed on the initialization
	int m_numberOfGameObjects; //number of game objects to order the creation and manage from the C++ code, passed on the initialization
	UnityArray<Vec2> m_gameObjectPositions; //shared array containing the position of each managed game object.
	vector<Vec2> m_gameObjectVelocities; //vector containing the velocity of each managed game object.
	
	double m_timeSinceStart;
	double m_timeOfLastRandomUpdates;
	double m_timeOfLastLogRelatedMessage; //so we avoid sending messages the result in logging calls all the time, what would compromise the performance
	float m_randomUpdatesInterval; //passed on initialization, positions are always updated, orther stuff are randomly updated after this interval

	int m_receiverIdForReflectionTest; //pick an instead game object receiver id just for testing reflection based messages on it

	static UnityForCppTest* s_pInstance;
}; //UnityForCppTest class


#endif