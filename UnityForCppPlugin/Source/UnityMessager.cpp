//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#include "UnityMessager.h"


#define UM_MIN_ALLOWED_VALUE_FOR_RECEIVER_IDS 16
#define UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE 512

#define UM_MESSAGE_BASE_LENGTH 3

#define UM_CONTROL_QUEUE_ID 0

#define UM_EMPTY_CONTROL_QUEUE_CODE -123456

namespace UnityForCpp
{

UnityMessager* UnityMessager::s_pInstance = NULL;

int UnityMessager::InstanceAndProvideAwakeInfo(int maxNOfReceiverIds, int maxQueueArraysSizeInBytes)
{
	if (maxNOfReceiverIds < UM_MIN_ALLOWED_VALUE_FOR_RECEIVER_IDS)
	{
		WARNING_LOGP("UnityMessager min value for maxNOfReceiverIds is %d. This value will be forced.", UM_MIN_ALLOWED_VALUE_FOR_RECEIVER_IDS);
		maxNOfReceiverIds = UM_MIN_ALLOWED_VALUE_FOR_RECEIVER_IDS;
	}

	if (maxQueueArraysSizeInBytes < UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE)
	{
		WARNING_LOGP("UnityMessager min value for maxQueueArraysSizeInBytes is %d. This value will be forced.", UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE);
		maxQueueArraysSizeInBytes = UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE;
	}
	
	ASSERT(s_pInstance == NULL);
	s_pInstance = new UnityMessager(maxNOfReceiverIds, maxQueueArraysSizeInBytes);

	return s_pInstance->ProvideUnityMessagerAwakeInfo();
}

UnityMessager::UnityMessager(int maxNOfReceiverIds, int maxQueueArraysSizeInBytes)
	: m_receiverIds(), m_pControlQueue(NULL), 
	m_maxQueueArraysSizeInBytes(UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE),
	m_lastAssignedQueueId(-1)
{
	ASSERT(maxNOfReceiverIds >= UM_MIN_ALLOWED_VALUE_FOR_RECEIVER_IDS
		&& maxQueueArraysSizeInBytes >= UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE);

	m_maxQueueArraysSizeInBytes = maxQueueArraysSizeInBytes;
	m_receiverIds.Alloc(maxNOfReceiverIds);
	for (int i = 0; i < maxNOfReceiverIds; ++i)
		m_receiverIds[i] = i + 1;

	m_receiverIds[maxNOfReceiverIds - 1] = 0;

	for (int i = 0; i < UM_MAX_N_OF_MESSAGE_QUEUES; ++i)
		m_messageQueuesPtrs[i] = NULL;

	s_pInstance = this; //We set this here before the suitable set where "new" is called because the queue instanciation bellow needs it

	m_pControlQueue = new ControlQueue();
	ASSERT(m_pControlQueue->GetQueueId() == UM_CONTROL_QUEUE_ID); //CONTROL QUEUE MUST BE THE QUEUE 0
	m_messageQueuesPtrs[UM_CONTROL_QUEUE_ID] = m_pControlQueue;
	m_lastAssignedQueueId = UM_CONTROL_QUEUE_ID;
}

void UnityMessager::DeleteInstance()
{
	DELETE(s_pInstance);
}

UnityMessager::~UnityMessager() 
{
	m_pControlQueue = NULL; //we delete it bellow

	for (int i = 0; i < UM_MAX_N_OF_MESSAGE_QUEUES; ++i)
		DELETE(m_messageQueuesPtrs[i]);

	m_lastAssignedQueueId = -1;

	s_pInstance = NULL;
}

int UnityMessager::NewReceiverId()
{
	int nextFreeId = m_receiverIds[0];
	m_receiverIds[0] = m_receiverIds[nextFreeId];
	m_receiverIds[nextFreeId] = -1;
	
	ASSERT(nextFreeId != 0);
	return nextFreeId;
}

int UnityMessager::ProvideUnityMessagerAwakeInfo()
{
	int params[] = { m_receiverIds.GetId() };
	m_pControlQueue->SendControlMessage(UMM_SET_RECEIVER_IDS_ARRAY, 1, params);

	return m_pControlQueue->GetFirstArrayId();
}

void UnityMessager::SendMessage(int receiverId, int msgId)
{
	m_pControlQueue->SendMessage(receiverId, msgId);
}

void UnityMessager::OnStartMessageDelivering()
{
	m_pControlQueue->SendControlMessage(UMM_FINISH_DELIVERING_MESSAGES, 0, NULL);

	for (int i = 0; i <= m_lastAssignedQueueId; ++i)
		m_messageQueuesPtrs[i]->Reset();
}

void UnityMessager::RegisterMessageQueue(int queueId, MessageQueueBase* pMsgQueue)
{
	if (queueId == UM_CONTROL_QUEUE_ID)
		return;

	ASSERT(queueId < UM_MAX_N_OF_MESSAGE_QUEUES);
	m_messageQueuesPtrs[queueId] = pMsgQueue;

	int params[] = { queueId, pMsgQueue->GetFirstArrayId() };
	m_pControlQueue->SendControlMessage(UMM_SET_QUEUE_FIRST_ARRAY, 2, params);
}

void UnityMessager::ReleasePossibleQueueArrays()
{
	ASSERT(!m_pControlQueue->IsThereAnyMessageToDeliver());
	for (int i = 0; i <= m_lastAssignedQueueId; ++i)
		m_messageQueuesPtrs[i]->ReleaseArraysExceptFirst();
}

UnityMessager::ControlQueue::ControlQueue()
	: MessageQueue<int>(), m_pCurrentNOfParams(NULL), isAdvancingToNextNode(false)
{
	m_pCurrentNode->unityArray[0] = UM_EMPTY_CONTROL_QUEUE_CODE;
}

void UnityMessager::ControlQueue::SendControlMessage(int msgId, int nOfParams, const int* intParams)
{
	int* controlQueueRef = AllocSpace(UM_MESSAGE_BASE_LENGTH + nOfParams);
	controlQueueRef[0] = UMR_MESSAGER;
	controlQueueRef[1] = msgId;
	controlQueueRef[2] = -nOfParams;
	for (int i = 0; i < nOfParams; ++i)
		controlQueueRef[3 + i] = intParams[i];
}

inline bool UnityMessager::ControlQueue::IsThereAnyMessageToDeliver()
{
	return m_pFirstNode->unityArray[0] != UM_EMPTY_CONTROL_QUEUE_CODE;
}

void UnityMessager::ControlQueue::AdvanceToNextUnityArrayNode()
{
	if (isAdvancingToNextNode)
		return;

	isAdvancingToNextNode = true;
	MessageQueue<int>::AdvanceToNextUnityArrayNode();
	isAdvancingToNextNode = false;
}


} //UnityForCpp




