//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

#ifndef UNITY_MESSAGER_HPP
#define UNITY_MESSAGER_HPP

#include "UnityMessager.h"
#include "UnityArray.h"
#include <string>

//This is the "Unity Messager Receiver" id for the UnityMessager instance itself (at the C# side)
#define UMR_MESSAGER 0

//"Unity Messager Receiver" defined messages for the UnityMessager instance itself (at the C# side)
enum UmrMessagerMessages {
	UMM_SET_QUEUE_ARRAY = 0, //(int queueId, int arrayId) => Sets the id of the next array to be used by the specified message queue 
	UMM_SET_QUEUE_FIRST_ARRAY = 1,//(int queueId, int arrayId) => Sets the id for the first array of a message queue, which never changes
	UMM_SET_RECEIVER_IDS_ARRAY = 2,//(int arrayId) => Sets the id for the array used to control the available receiver ids
	UMM_FINISH_DELIVERING_MESSAGES = 3, //() => Sets the finish point for the delivering message process.
	UMM_REGISTER_NEW_COMPONENT = 4
};

namespace UnityForCpp
{

template<typename... PARAMS>
void UnityMessager::SendMessage(int receiverId, int msgId, const PARAMS&... params)
{
	m_pControlQueue->SendMessage(receiverId, msgId);
	PushParam(params...);
}

template<typename COMPONENT_TYPE, typename... PARAMS>
void UnityMessager::SendMessage(int receiverId, int msgId, const PARAMS&... params)
{
	m_pControlQueue->SendMessage(receiverId, COMPONENT_TYPE::GetId(), msgId);
	PushParam(params...);
}

template<typename COMPONENT_TYPE, typename... PARAMS>
void UnityMessager::SendMessage(int receiverId, const char* methodName, const PARAMS&... params)
{
	int componentId = COMPONENT_TYPE::GetId(); //MUST COME BEFORE modifing any message queue, since this may call SendMessage internally
	int methodNameLength = strlen(methodName);
	
	//Push the method name direcly on the Byte parameter queue, but without registering it as a parameter
	ParamQueue<uint8>::GetInstance().Push((const uint8*)methodName, methodNameLength);

	//We mark we are using reflection instead of a usual message id by setting the negative method name length as the message id
	m_pControlQueue->SendMessage(receiverId, componentId, -methodNameLength);

	PushParam(params...);
}

template<typename COMPONENT_TYPE, typename... PARAMS> 
void UnityMessager::SendMessage(const char* objectName, int msgId, const PARAMS&... params)
{
	int componentId = COMPONENT_TYPE::GetId(); //MUST COME BEFORE modifing any message queue, since this may call SendMessage internally
	int objectNameLength = strlen(objectName);

	//Push the object name direcly on the Byte parameter queue, but without registering it as a parameter
	ParamQueue<uint8>::GetInstance().Push((const uint8*)objectName, objectNameLength);

	//We mark we are using reflection instead of a usual receiver id by setting the negative object name length as the receiver id
	m_pControlQueue->SendMessage(-objectNameLength, componentId, msgId);

	PushParam(params...);
}

template<typename COMPONENT_TYPE, typename... PARAMS> 
void UnityMessager::SendMessage(const char* objectName, const char* methodName, const PARAMS&... params)
{
	int componentId = COMPONENT_TYPE::GetId(); //MUST COME BEFORE modifing any message queue, since this may call SendMessage internally

	//Push the object name direcly on the Byte parameter queue, but without registering it as a parameter
	int objectNameLength = strlen(objectName);
	ParamQueue<uint8>::GetInstance().Push((const uint8*)objectName, objectNameLength);

	//Push the method name direcly on the Byte parameter queue, but without registering it as a parameter
	int methodNameLength = strlen(methodName);
	ParamQueue<uint8>::GetInstance().Push((const uint8*)methodName, methodNameLength);

	//We mark where strings are used instead of the usual ids by setting their negative lengths instead as the ids
	m_pControlQueue->SendMessage(-objectNameLength, componentId, -methodNameLength);

	PushParam(params...);
}


inline void UnityMessager::PushParam(const char* stringParam)
{	//a C string is just pushed as an uint8 array (System.Byte array at the C# side)
	PushParam(UM_ARRAY_PARAM((const uint8*)stringParam, strlen(stringParam)));
}

template <typename T> 
inline void UnityMessager::PushParam(const ArrayParam<T>& arrayParam)
{
	ParamQueue<T>::GetInstance().Push(arrayParam.pArray, arrayParam.length);
	m_pControlQueue->RegisterParam(ParamQueue<T>::GetInstance().GetQueueId(), arrayParam.length);
}

template <typename T>
inline void UnityMessager::PushParam(const ArrayToFillParam<T>& arrayToFillParam)
{
	ParamQueue<T>::GetInstance().PushAndGetPtrToFill(&(arrayToFillParam.m_pArray), arrayToFillParam.GetLength());
	m_pControlQueue->RegisterParam(ParamQueue<T>::GetInstance().GetQueueId(), arrayToFillParam.GetLength());
}

template <typename T>
inline void UnityMessager::PushParam(const T& param)
{
	ParamQueue<T>::GetInstance().Push(param);
	m_pControlQueue->RegisterParam(ParamQueue<T>::GetInstance().GetQueueId());
}

template <typename PARAM1, typename... OTHER_PARAMS>
inline void UnityMessager::PushParam(const PARAM1& param1, const OTHER_PARAMS&... otherParams)
{
	PushParam(param1);
	PushParam(otherParams...);
}

template <typename T>
inline T* UnityMessager::MessageQueue<T>::AllocSpace(int length)
{
	//If we don't have enough space on the current array, we need to advance to the next array of the queue.
	if (m_currentArrayPos + length > m_pCurrentNode->unityArray.GetLength())
	{
		//We can never push an array with bigger length than the UnityArray length for a message queue. 
		//An isolated UnityArray must be used on these cases, having its id passed as int parameter.
		ASSERT(length < m_pCurrentNode->unityArray.GetLength());
		AdvanceToNextUnityArrayNode();
	}

	m_currentArrayPos += length;
	return &(m_pCurrentNode->unityArray[m_currentArrayPos - length]);
}

template <typename T>
UnityMessager::MessageQueue<T>::MessageQueue()
	: m_pFirstNode(NULL), m_pCurrentNode(NULL), m_currentArrayPos(0), m_queueId(-1)
{
	UnityMessager& unityMessager = UnityMessager::GetInstance();

	m_pFirstNode = new Node(unityMessager.GetMaxQueueArraysSizeInBytes() / sizeof(T));
	m_pCurrentNode = m_pFirstNode;
	m_queueId = unityMessager.GetNextFreeQueueIdAndIncrement();
	unityMessager.RegisterMessageQueue(m_queueId, this);
}

template <typename T>
UnityMessager::MessageQueue<T>::~MessageQueue()
{
	m_pCurrentNode = NULL;
	while (m_pFirstNode) //single linked list destruction
	{
		Node* pToDelete = m_pFirstNode;
		m_pFirstNode = m_pFirstNode->pNext;
		DELETE(pToDelete);
	}
}

template <typename T>
void UnityMessager::MessageQueue<T>::AdvanceToNextUnityArrayNode()
{
	if (m_pCurrentNode->pNext == NULL) //we may have it already created from previous usages
		m_pCurrentNode->pNext = new Node(m_pFirstNode->unityArray.GetLength());

	//Sends the control message that goes in front of the new value being set, in such way the 
	//UnityMessager instance at the C# side changes the current array instance before reading values from the
	//respective message queue. 
	int msgParams[2] = { m_queueId, m_pCurrentNode->pNext->unityArray.GetId() };
	UnityMessager::GetInstance().m_pControlQueue->SendControlMessage(UMM_SET_QUEUE_ARRAY, 2, msgParams);

	//observe we advance to the next node (array) ONLY AFTER we have sent the control message
	m_pCurrentNode = m_pCurrentNode->pNext;
	m_currentArrayPos = 0;
}

template <typename T>
inline void UnityMessager::MessageQueue<T>::Reset()
{
	m_pCurrentNode = m_pFirstNode;
	m_currentArrayPos = 0;
}

template <typename T>
inline bool UnityMessager::MessageQueue<T>::IsReset()
{
	return m_pFirstNode == m_pCurrentNode && m_currentArrayPos == 0;
}

template <typename T>
void UnityMessager::MessageQueue<T>::ReleaseArraysExceptFirst()
{
	ASSERT(IsReset());
	Node* pNextToDelete = m_pFirstNode->pNext;
	m_pFirstNode->pNext = NULL;
	while (pNextToDelete) //single linked list destruction (except by the first element)
	{
		Node* pToDelete = pNextToDelete;
		pNextToDelete = pNextToDelete->pNext;
		DELETE(pToDelete);
	}
}

inline void UnityMessager::ControlQueue::SendMessage(int receiverId, int msgId)
{
	int* controlQueueRef = AllocSpace(3);
	m_pCurrentNOfParams = &(controlQueueRef[2]); //pointer to the value to increment when parameters a pushed

	controlQueueRef[0] = receiverId;
	controlQueueRef[1] = msgId;
	controlQueueRef[2] = 0; //number of parameters pointed by m_pCurrentNOfParams
}

inline void UnityMessager::ControlQueue::SendMessage(int receiverId, int componentId, int msgId)
{
	int* controlQueueRef = AllocSpace(4);
	m_pCurrentNOfParams = &(controlQueueRef[2]); //pointer to the value to increment when parameters a pushed

	controlQueueRef[0] = receiverId;
	controlQueueRef[1] = msgId;

	//A negative number of parameters (incremented by -1 to assure it's negative) is a sign to indicate the next value is the component id
	controlQueueRef[2] = -1; 
	controlQueueRef[3] = componentId;
}

inline void UnityMessager::ControlQueue::IncrementNumberOfParamsForCurrentMessage()
{
	if (*m_pCurrentNOfParams >= 0) //a negative number of params (incremented by -1) means a message to a game object component 
		++(*m_pCurrentNOfParams); //so we just increment if it is positive and decrement if it is negative to keep the sign.
	else
		--(*m_pCurrentNOfParams);
}

inline void UnityMessager::ControlQueue::RegisterParam(int queueId)
{
	int* controlQueueRef = AllocSpace(1);
	controlQueueRef[0] = queueId;
	IncrementNumberOfParamsForCurrentMessage();
}

inline void UnityMessager::ControlQueue::RegisterParam(int queueId, int length)
{
	int* controlQueueRef = AllocSpace(2);
	controlQueueRef[0] = - queueId; //NEGATIVE queueId VALUE IS THE SIGN FOR AN ARRAY PARAMETER INSTEAD OF A SINGLE PARAMETER
	controlQueueRef[1] = length; //SO THE C# CONTROL QUEUE INSTANCE KNOWS THE ARRAY LENGTH COMES NEXT 
	IncrementNumberOfParamsForCurrentMessage();
}

#define UM_SET_QUEUE_ARRAY_MSG_LENGTH 5

inline int* UnityMessager::ControlQueue::AllocSpace(int length)
{
	//The control queue makes sure to save space for a "set queue array" message, needed when the control queue 
	//itself gets out of space and needs to be advanced. 
	if (m_currentArrayPos + length > m_pCurrentNode->unityArray.GetLength() - UM_SET_QUEUE_ARRAY_MSG_LENGTH)
		AdvanceToNextUnityArrayNode();

	m_currentArrayPos += length;
	return &(m_pCurrentNode->unityArray[m_currentArrayPos - length]);
}

template <typename T> UnityMessager::ParamQueue<T>* UnityMessager::ParamQueue<T>::s_pInstance = NULL;

template <typename T>
inline void UnityMessager::ParamQueue<T>::Push(const T& param)
{
	T* pParamDataDest = AllocSpace(1);
	*pParamDataDest = param;
}

template <typename T>
inline void UnityMessager::ParamQueue<T>::Push(const T* pItemArray, int length)
{
	T* pParamDataDest = AllocSpace(length);
	memcpy(pParamDataDest, pItemArray, length*sizeof(T));
}

template <typename T>
inline void UnityMessager::ParamQueue<T>::PushAndGetPtrToFill(T** const ppQueueSubArrayOutput, int length)
{
	//Just pass the pointer for the array to be filled, here we have nothing to fill.
	(*ppQueueSubArrayOutput) = AllocSpace(length);
}

template <typename T>
int UnityMessager::Component<T>::s_id = -1; //-1 means the component was not registered yet, this triggers the register method

template <typename T>
int UnityMessager::Component<T>::GenerateSetAndGetComponentId()
{
	//send the message to get it registered for this id and managed type name before any use
	UNITY_MESSAGER.RegisterNewComponent(T::GetManagedTypeName(), &s_id);
	return s_id;
}

} //UnityForCpp

#endif


