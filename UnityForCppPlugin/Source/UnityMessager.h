//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#ifndef UNITY_MESSAGER_H
#define UNITY_MESSAGER_H

#include "Shared.h"
#include "UnityArray.h"

#define UNITY_MESSAGER UnityForCpp::UnityMessager::GetInstance()

#define UM_ARRAY_PARAM(arrayPtr, length) UnityForCpp::UnityMessager:: \
											ArrayParam< std::remove_const< std::remove_pointer< decltype(&(*(arrayPtr))) >:: \
													type >::type >(&(arrayPtr)[0], length)

#define UM_CREATE_ARRAY_TO_FILL_PARAM(type, length) UnityForCpp::UnityMessager::ArrayToFillParam<type>::CreateArrayToFill(length);

#define UM_MAX_N_OF_MESSAGE_QUEUES 16

#define UMR_MESSAGER 0

enum UmrMessagerMessages {
	UMM_SET_QUEUE_ARRAY = 0,
	UMM_SET_QUEUE_FIRST_ARRAY = 1,
	UMM_SET_RECEIVER_IDS_ARRAY = 2,
	UMM_FINISH_DELIVERING_MESSAGES = 3
};

namespace UnityForCpp
{

class UnityMessager
{
public:
	static UnityMessager& GetInstance() { ASSERT(s_pInstance); return *s_pInstance; }

	int NewReceiverId();
	
	void SendMessage(int receiverId, int msgId);
	template<typename... PARAMS> void SendMessage(int receiverId, int msgId, const PARAMS&... params);

	template <typename T>
	struct ArrayParam
	{
		const T* pArray;
		int length;

		ArrayParam(const T* arrayPtr, int arrayLength)
			: pArray(arrayPtr), length(arrayLength) {}
	};

	template <typename T>
	class ArrayToFillParam
	{
	public:
		static ArrayToFillParam<T> CreateArrayToFill(int length) { return ArrayToFillParam(length); }
		
		int GetLength() const { return m_length; }
		T& operator[](int idx)
		{
			ASSERT(m_pArray && idx >= 0 && idx < m_length);
			return m_pArray[idx];
		}

	private:
		ArrayToFillParam(int length)
			: m_pArray(NULL), m_length(length) {}
		
		mutable T* m_pArray;
		int m_length;

		friend class UnityMessager;
	};

	void ReleasePossibleQueueArrays();
	void OnStartMessageDelivering();
	static int InstanceAndProvideAwakeInfo(int maxNOfReceiverIds, int maxQueueArraysSizeInBytes);
	static void DeleteInstance();

private:
	class MessageQueueBase; //foward declaration
	template <typename T> class MessageQueue;
	class ControlQueue;

	static UnityMessager* s_pInstance;

	UnityMessager(int maxNOfReceiverIds, int maxQueueArraysSizeInBytes);
	~UnityMessager();

	void PushParam(const char* stringParam);
	template <typename T> void PushParam(const ArrayParam<T>& arrayParam);
	template <typename T> void PushParam(const ArrayToFillParam<T>& arrayParam); 
	template <typename T> void PushParam(const T& param);
	template <typename PARAM1, typename... OTHER_PARAMS> void PushParam(const PARAM1& param1, const OTHER_PARAMS&... otherParams);

	int ProvideUnityMessagerAwakeInfo();

	int GetMaxQueueArraysSizeInBytes() { return m_maxQueueArraysSizeInBytes; }
	int GetNextFreeQueueIdAndIncrement() { return ++m_lastAssignedQueueId; }
	void RegisterMessageQueue(int queueId, MessageQueueBase* msgQueue);

	UnityArray<int> m_receiverIds;

	MessageQueueBase* m_messageQueuesPtrs[UM_MAX_N_OF_MESSAGE_QUEUES];
	ControlQueue* m_pControlQueue;

	int m_maxQueueArraysSizeInBytes;
	int m_lastAssignedQueueId;

	class MessageQueueBase
	{
	public:
		virtual ~MessageQueueBase() {}
		virtual int GetFirstArrayId() = 0;
		virtual void Reset() = 0;
		virtual void ReleaseArraysExceptFirst() = 0;
	};

	template <typename T>
	class MessageQueue : public MessageQueueBase
	{
	public:
		MessageQueue();
		virtual ~MessageQueue();

		int GetQueueId() { return m_queueId; }
		int GetFirstArrayId() { return m_pFirstNode->unityArray.GetId(); }

		bool IsReset();
		
		virtual void Reset();
		virtual void ReleaseArraysExceptFirst();

	protected:
		T* AllocSpace(int length);
		void AdvanceToNextUnityArrayNode();

		typedef struct Node
		{
			UnityArray<T> unityArray;
			Node *pNext;

			Node(int length)
				: unityArray(), pNext(NULL) 
			{
				unityArray.Alloc(length);
			}

		} Node;

		Node* m_pFirstNode;
		Node* m_pCurrentNode;
		int m_currentArrayPos;
		int m_queueId;
	};

	class ControlQueue : public MessageQueue<int>
	{
	public:
		ControlQueue();
		bool IsThereAnyMessageToDeliver();
		
		void SendMessage(int receiverId, int msgId);
		void SendControlMessage(int msgId, int nOfParams, const int* intParams);
		void RegisterParam(int queueId);
		void RegisterParam(int queueId, int length);

	private:
		int* AllocSpace(int length);
		void AdvanceToNextUnityArrayNode();

		int *m_pCurrentNOfParams;
		bool isAdvancingToNextNode;
	};

	template <typename T> 
	class ParamQueue : public MessageQueue<T>
	{
	public:
		static ParamQueue<T>& GetInstance() { return *(s_pInstance ? s_pInstance : (s_pInstance = new ParamQueue<T>())); }
		virtual ~ParamQueue() { s_pInstance = NULL; }

		void Push(const T& item);
		void Push(const T* pItemArray, int length);
		void PushAndGetPtrToFill(T** const ppQueueSubArrayOutput, int length);

	protected:
		using MessageQueue<T>::AllocSpace;

	private:
		ParamQueue() {}
		static ParamQueue<T>* s_pInstance;
	};
};

}; //UnityForCpp

#include "UnityMessager.hpp"

#endif

