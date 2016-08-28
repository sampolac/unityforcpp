//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

#include "UnityMessager.h"


#define UM_MIN_ALLOWED_VALUE_FOR_RECEIVER_IDS 16
#define UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE 512

//the message base is defined by (receiverId, messagerId, numberOfParameters)
#define UM_MESSAGE_BASE_LENGTH 3

//this corresponds to UnityMessager._controlQueueId on C#
#define UM_CONTROL_QUEUE_ID 0

//This code is set by the C# code to the first position of the first array of the control queue in order to
//set that all messages were delivered, in such way the control queue can be considered empty. 
//UnityMessager.ControlQueue._emptyCode is the corresponding constant on the C# code.
#define UM_EMPTY_CONTROL_QUEUE_CODE -123456

namespace UnityForCpp
{

UnityMessager* UnityMessager::s_pInstance = NULL;

int UnityMessager::InstanceAndProvideAwakeInfo(int maxNOfReceiverIds, int maxQueueArraysSizeInBytes)
{
	if (maxNOfReceiverIds < UM_MIN_ALLOWED_VALUE_FOR_RECEIVER_IDS)
	{
		WARNING_LOGF("UnityMessager min value for maxNOfReceiverIds is %d. This value will be forced.", UM_MIN_ALLOWED_VALUE_FOR_RECEIVER_IDS);
		maxNOfReceiverIds = UM_MIN_ALLOWED_VALUE_FOR_RECEIVER_IDS;
	}

	if (maxQueueArraysSizeInBytes < UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE)
	{
		WARNING_LOGF("UnityMessager min value for maxQueueArraysSizeInBytes is %d. This value will be forced.", UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE);
		maxQueueArraysSizeInBytes = UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE;
	}
	
	ASSERT(s_pInstance == NULL);

	//We make this assignment here since it is the logical place for it to be. However, we know the UnityMessager constructor
	//will do this assignment itself because s_pInstance must be valid when the control queue is beign instanced. 
	s_pInstance = new UnityMessager(maxNOfReceiverIds, maxQueueArraysSizeInBytes);

	return s_pInstance->ProvideUnityMessagerAwakeInfo();
}

UnityMessager::UnityMessager(int maxNOfReceiverIds, int maxQueueArraysSizeInBytes)
	: m_receiverIds(), m_lastAssignedComponentId(-1), m_pControlQueue(NULL),
	m_maxQueueArraysSizeInBytes(UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE),
	m_lastAssignedQueueId(-1)
{
	ASSERT(maxNOfReceiverIds >= UM_MIN_ALLOWED_VALUE_FOR_RECEIVER_IDS
		&& maxQueueArraysSizeInBytes >= UM_MIN_ALLOWED_VALUE_FOR_QUEUE_ARRAY_SIZE);

	m_maxQueueArraysSizeInBytes = maxQueueArraysSizeInBytes;
	m_receiverIds.Alloc(maxNOfReceiverIds); //instances the shared array to control the receiver ids.

	//The position 0 of the m_receiverIds shared array indicates the NEXT FREE receiver id, and at the position
	//of this given id on the m_receiverIds shared array we set the next free id after that. So, it is a kind of 
	//simple linked list of free receiver ids stablished by the m_receiverIds array. Check the comments for the 
	//implementation of NewReceiverId() for more details about how the control of the free receiver ids is made. 
	for (int i = 0; i < maxNOfReceiverIds; ++i)
		m_receiverIds[i] = i + 1; //"i+1" is the next free id after the initialization, indicated by the i position. 

	m_receiverIds[maxNOfReceiverIds - 1] = 0; //0 means there is no other free id beyond this one.

	for (int i = 0; i < UM_MAX_N_OF_COMPONENTS; ++i)
		m_staticComponentIdsToResetPtrs[i] = NULL;

	//initialize the message queues array. Except for the ControlQueue the other message queues are ParamQueue<T> objects
	//that get instanced only at the first time a parameter T is pushed to a message in a given game execution.
	for (int i = 0; i < UM_MAX_N_OF_MESSAGE_QUEUES; ++i)
		m_messageQueuesPtrs[i] = NULL;

	//We set this here because it is needed for the ControlQueue instance creation right bellow. 
	s_pInstance = this; 

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
	//reset component ids to -1, so next time they are used (happens on Unity Editor) the registering process is repeated
	for (int i = 0; i <= m_lastAssignedComponentId; ++i)
		*(m_staticComponentIdsToResetPtrs[i]) = -1;

	m_pControlQueue = NULL; //we delete it bellow
	for (int i = 0; i <= m_lastAssignedQueueId; ++i)
		DELETE(m_messageQueuesPtrs[i]);

	s_pInstance = NULL;
}

int UnityMessager::NewReceiverId()
{
	int nextFreeId = m_receiverIds[0]; //Position 0 holder the next free id

	//The next free id holds the next free id after it, so bellow we are removing the next free id from the free ids single linked list
	m_receiverIds[0] = m_receiverIds[nextFreeId]; 
	// -1 means this id is use, not being free anymore. ONLY THE C# CODE CAN RELEASE IDs, so they get back to the single linked list.
	m_receiverIds[nextFreeId] = -1;
	
	ASSERT(nextFreeId != 0); //IF IT FAILS WE GOT OUT OF RECEIVER IDS, this must not happen!!!
	return nextFreeId;
}

int UnityMessager::ProvideUnityMessagerAwakeInfo()
{
	//For now this is the only message we need to send to initialize the C# UnityMessager instace
	int params[] = { m_receiverIds.GetId() };
	m_pControlQueue->SendControlMessage(UMM_SET_RECEIVER_IDS_ARRAY, 1, params);

	return m_pControlQueue->GetFirstArrayId();
}

void UnityMessager::OnStartMessageDelivering()
{
	m_pControlQueue->SendMessage(0, UMM_FINISH_DELIVERING_MESSAGES);

	//Reset all the queues, preparing them for the next usage which should happen only after all messages get delivered
	for (int i = 0; i <= m_lastAssignedQueueId; ++i)
		m_messageQueuesPtrs[i]->Reset();
}

void UnityMessager::RegisterNewComponent(const char* componentTypeName, int* componentIdStaticPtr)
{
	*componentIdStaticPtr = ++m_lastAssignedComponentId;
	ASSERT(m_lastAssignedComponentId < UM_MAX_N_OF_COMPONENTS);

	SendMessage(0, UMM_REGISTER_NEW_COMPONENT, m_lastAssignedComponentId, componentTypeName);
	
	//store the static variable address to we can reset it when UnityMessager instance is destroyed
	m_staticComponentIdsToResetPtrs[m_lastAssignedComponentId] = componentIdStaticPtr;
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
	if (m_pControlQueue->IsThereAnyMessageToDeliver())
	{
		ASSERT(false);
		return;
	}

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
	controlQueueRef[0] = UMR_MESSAGER; //(0)
	controlQueueRef[1] = msgId;
	controlQueueRef[2] = -nOfParams; //THIS, PLUS receiverId == 0 (UMR_MESSAGER), INDICATES A CONTROL MESSAGE 
	
	//control messages only support int parameters, which are pushed directly to the control queue.
	for (int i = 0; i < nOfParams; ++i)
		controlQueueRef[3 + i] = intParams[i];
}

inline bool UnityMessager::ControlQueue::IsThereAnyMessageToDeliver()
{
	return m_pFirstNode->unityArray[0] != UM_EMPTY_CONTROL_QUEUE_CODE;
}

void UnityMessager::ControlQueue::AdvanceToNextUnityArrayNode()
{
	//avoids an endless recursion since the control queue SendMessage call to advance itself must not result on another
	//call for advancing itself again. The ControlQueue::AllocSpace method assures the current array of the control queue
	//will always reserve space for sending the message the sets the next control queue array on C#. 
	if (isAdvancingToNextNode)
		return;

	isAdvancingToNextNode = true;
	MessageQueue<int>::AdvanceToNextUnityArrayNode();
	isAdvancingToNextNode = false;
}


} //UnityForCpp




