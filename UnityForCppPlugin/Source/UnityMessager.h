//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

#ifndef UNITY_MESSAGER_H
#define UNITY_MESSAGER_H

#include "Shared.h"
#include "UnityArray.h"

//Alternative access point to the UnityMessager singleton instance.
#define UNITY_MESSAGER UnityForCpp::UnityMessager::GetInstance()

//Use this to DIRECTLY on the parameter list when calling SendMessage in order to pass an array parameter for an existing C array.
#define UM_ARRAY_PARAM(arrayPtr, length) UnityForCpp::UnityMessager:: \
											ArrayParam< std::remove_const< std::remove_pointer< decltype(&(*(arrayPtr))) >:: \
													type >::type >(&(arrayPtr)[0], length)

//Use this to create an ArrayToFillParam instance you can use to pass as parameter on a SendMessage call and after that (AND ONLY
//AFTER THAT) fill the array yourself item by item, without the need of creating a temporary C array. 
//RECOMMENDED USAGE: auto myArrayToBeFilled == UM_CREATE_ARRAY_TO_FILL_PARAM(type, length);
#define UM_CREATE_ARRAY_TO_FILL_PARAM(type, length) UnityForCpp::UnityMessager::ArrayToFillParam<type>::CreateArrayToFill(length);

//This sets the suport for (UM_MAX_N_OF_MESSAGE_QUEUES - 1) different parameter types. If you exceed that a C assertion will
//be triggered so you know you need to increase this value and recompile your code. However, with that set to 32 it is almost
//impossible to have this value not being enough for anyone's needs.
//It corresponds to UnityMessager._maxNOfMessageQueues on C#, so once you update this value here be sure to update there too.
#define UM_MAX_N_OF_MESSAGE_QUEUES 32

namespace UnityForCpp
{

class UnityMessager
{
public:
	//Singleton access point. the UNITY_MESSAGER macro may be used instead for your convenience.
	static UnityMessager& GetInstance() { ASSERT(s_pInstance); return *s_pInstance; }

	//Use this method when ordering the creation of a new receiver C# object from the C++ code. You usually will be sending
	// this id as parameter to the "create" message, for which the receiver is the factory of this new receiver C# object, 
	//which should be set as receiver bound to this passed id on the C# UnityMessager instance. This way you can send
	//messages to the newly created receiver object *IMMEDIATELY* after the create message, EVEN knowing the C# instance of this
	//new C# receiver instance doesn't exist yet, since the create message was not delivered yet. 
	int NewReceiverId();
	
	//Sends a message with no parameters to the C# object set to the specificed receiverId.
	//The message id (msgId) value is totally under your control for you to route received messages on your C# code.
	//YOU CAN NEVER SEND NEW MESSAGES DURING THE MESSAGE DELIVERING PROCESS STARTED BY THE C# CODE, BE AWARE OF THAT
	//FOR THE CASE YOUR C# MESSAGE HANDLE CODE INVOKES C++ CODE, SO YOU DON'T SEND MESSAGES FROM THEM!
	void SendMessage(int receiverId, int msgId);

	//Sends a message with any number of parameters. Any type supported by UnityArray is also supported here (check  
	//UA_SUPPORTED_TYPE comments), as well as arrays of these types passed using an instance of the struct ArrayParam or 
	//of the class ArrayToFillParam (check the comments of these). Also C strings are supported (char* or const char*), 
	//being packed to a byte array (uint8) that can be read as string when unpacking the message on the C# code.
	//YOU CAN NEVER SEND NEW MESSAGES DURING THE MESSAGE DELIVERING PROCESS STARTED BY THE C# CODE, BE AWARE OF THAT
	//FOR THE CASE YOUR C# MESSAGE HANDLE CODE INVOKES C++ CODE, SO YOU DON'T SEND MESSAGES FROM THEM!
	template<typename... PARAMS> void SendMessage(int receiverId, int msgId, const PARAMS&... params);

	//Simple struct for used to push array parameters by wrapping a C array pointer together with its length in a single
	//parameter. Uses the macro UM_ARRAY_PARAM for instancing it directly when passing the parameters to SendMessage. 
	template <typename T>
	struct ArrayParam
	{
		const T* pArray;
		int length;

		//*USE* the macro UM_ARRAY_PARAM instead for your convenience.
		ArrayParam(const T* arrayPtr, int arrayLength)
			: pArray(arrayPtr), length(arrayLength) {}
	};


	//This class providers an alternative to ArrayParam, allowing you to push array parameters without the 
	//need of creating a temporary C array for when you don't have one. Using this class you will be setting
	//values directly on the shared array to be read when the C# code unpacks your message. 
	template <typename T>
	class ArrayToFillParam
	{
	public:
		//Creates a new ArrayToFillParam instance which MUST be passed as parameter on a SendMessage call BEFORE
		//BEING FILLED. The purpose of this static factory method is trying to prevent instances to be created 
		//on the heap since trying to keep an ArrayToFillParam instance beyond your method context is dangerous.
		//You may use the macro UM_CREATE_ARRAY_TO_FILL_PARAM for easily calling this method.
		static ArrayToFillParam<T> CreateArrayToFill(int length) { return ArrayToFillParam(length); }
		
		int GetLength() const { return m_length; }
		
		//YOU CAN ONLY USE THIS OPERATOR AFTER PUSHING THE INSTANCE AS PARAMETER TO A SendMessage CALL.
		T& operator[](int idx)
		{
			ASSERT(m_pArray && idx >= 0 && idx < m_length);
			return m_pArray[idx];
		}

	private: 
		ArrayToFillParam(int length)
			: m_pArray(NULL), m_length(length) {}
		
		//Parameters are always pushed as const & in order to avoid compilation issues with the variadic templates,
		//so we define m_pArray as mutable since we need to set it to point the first message queue position to be
		//filled as the corresponding parameter.
		mutable T* m_pArray;
		int m_length;

		friend class UnityMessager;
	};

	//This method MUST BE USED ONLY by the UnityMessagerPlugin interface implementation.
	//it may be called on situations where the C# code wants to free not essential memory being used by the game.
	//An usual case is when the app is paused (loses focus) on mobile devices.
	void ReleasePossibleQueueArrays();

	//This method MUST BE USED ONLY by the UnityMessagerPlugin interface implementation.
	//It must be called when the C# UnityMessage instance will start the message delivering process, so a last
	//message to mark the end of the delivering process can be send and also the C++ side to be reset since we know
	//all the messages will be delivered, which implies messages queues will be empty when sending the next message.
	void OnStartMessageDelivering();

	//This method MUST BE USED ONLY by the UnityMessagerPlugin interface implementation.
	//Creates the singleton instance and returns the id for the first array of the control queue, which should be 
	//returned to the calling C# code, in such way its UnityMessager instance can also be properly constructed.
	//- maxNOfReceiverIds defines the maximum number of receiver C# objects that can be used simultaneously, this
	//value cannot be changed until the next game execution. The min accepted value is 16. 
	//- maxQueueArraysSizeInBytes defines the maximum size in bytes for each array of each message queue, 
	//independently of the message queue type. With that you are actually defining the desired size for shared memory 
	//blocks used to send messages. 512 bytes is minimum, but 1024 or 2048 could be better values in many cases.
	static int InstanceAndProvideAwakeInfo(int maxNOfReceiverIds, int maxQueueArraysSizeInBytes);

	//This method MUST BE USED ONLY by the UnityMessagerPlugin interface implementation.
	//When OnDestroy is called at the UnityMessager C# instance, we need to delete the UnityMessager instance
	//so all the MessageQueue shared arrays in use are released. After DeleteInstance is called a new
	//singleton instance may be created on a new game execution via the Unity Editor.
	static void DeleteInstance();

private: //===================== END OF THE PUBLIC INTERFACE ====================================================
	class MessageQueueBase; //foward declarations
	template <typename T> class MessageQueue;
	class ControlQueue;

	static UnityMessager* s_pInstance; //pointer to the singleton instance

	//Singleton instance constructor, check InstanceAndProvideAwakeInfo comments for more details
	UnityMessager(int maxNOfReceiverIds, int maxQueueArraysSizeInBytes);
	~UnityMessager(); 

	//PushParam variations for different parameter types, all of them push a single parameter to
	//their respective ParamQueue and register this parameter existence on the control queue
	template <typename T> void PushParam(const T& param);
	void PushParam(const char* stringParam);
	template <typename T> void PushParam(const ArrayParam<T>& arrayParam);
	template <typename T> void PushParam(const ArrayToFillParam<T>& arrayParam); 

	//Variadic template version of PushParam, MAYBE IT SHOULD BE PUBLIC, so users can push parameters programatically
	//when the final number of parameters is not known in advance, IN OTHER HAND, in the great majority of these cases
	//using an array parameter is the proper usage, since pushing individual parameters instead, will overload the control queue
	template <typename PARAM1, typename... OTHER_PARAMS> void PushParam(const PARAM1& param1, const OTHER_PARAMS&... otherParams);

	//Send initial required messages to start the UnityMessager instance at the C# side and return the id for
	//the first array of the control queue, which is the access info needed for receiving the mentioned messages
	int ProvideUnityMessagerAwakeInfo();

	//This private getter method exists to be used directly by MessageQueue instances on their construction.  
	//This value is set on the singleton instance creation and is used as size reference for each new ParamQueue instanced.
	//By being a value in bytes, each queue may have a different length depending on their type size.
	int GetMaxQueueArraysSizeInBytes() { return m_maxQueueArraysSizeInBytes; }

	//This private getter method exists to be used directly by MessageQueue instances on their construction.  
	//It returns the next available queueId (index of the m_messageQueuesPtrs array), which must be used for registering the queue
	int GetNextFreeQueueIdAndIncrement() { return ++m_lastAssignedQueueId; }
	
	//This method exists to be used directly by MessageQueue instances on their construction.  
	//It register a new MessageQueue being instanced by saving it on the m_messageQueuesPtrs array
	void RegisterMessageQueue(int queueId, MessageQueueBase* msgQueue);

	//this array is shared with the C# code and implements a system to allows new receiver ids to be requested
	//by the C++ code or by the C# code, as the user logic requires, check NewReceiverId() implemention for details.
	UnityArray<int> m_receiverIds;

	//array keeping all the instaced messages queues, being the control queue at the position 0 and the ParamQueue for 
	//each different parameter type on following positions (order is determined by the usage order when pushing parameters).
	MessageQueueBase* m_messageQueuesPtrs[UM_MAX_N_OF_MESSAGE_QUEUES];

	//Control queue, unique during the UnityMessager instance lifetime, created with that and correponding to the
	//position 0 of the array m_messageQueuesPtrs.
	ControlQueue* m_pControlQueue;

	//check comments for GetMaxQueueArraysSizeInBytes
	int m_maxQueueArraysSizeInBytes;

	//when a new ParamQueue is instanced by its template dependent PushParam method, this attribute provides
	//the next available id on the m_messageQueuesPtrs array. 
	int m_lastAssignedQueueId;

	//Provides a polymorphic access point to MessageQueue instances for the methods required out of 
	//template dependent methods on UnityMessager. 
	class MessageQueueBase
	{
	public:
		virtual ~MessageQueueBase() {} //virtual so it can be used to properly delete MessageQueue instances 
		virtual int GetFirstArrayId() = 0; //check comments on the MessageQueue interface 
		virtual void Reset() = 0; //check comments on the MessageQueue interface 
		virtual void ReleaseArraysExceptFirst() = 0; //check comments on the MessageQueue interface 
	};

	//A message queue is a sequence of shared arrays between C# and C++ always filled from its first array
	//to its last one as queue and read by the C# code in the same way (FIFO: first in, first out).
	//The C++ side of a message queue is responsible for allocating new UnityArrays and controlling their 
	//change when the current one is full by sending control messages. 
	template <typename T>
	class MessageQueue : public MessageQueueBase
	{
	public:
		MessageQueue();

		//Instances live for a same game execution together with the UnityMessager instance. When the game 
		//will be closed or stopped on the Editor, all the message queues are deleted and will release their arrays. 
		virtual ~MessageQueue();

		//each MessageQueue instance has an unique queueId, corresponding to its position on the MessageQueueBase 
		//array at the UnityMessager singleton instance. The control queue must be always the id 0, while each
		//type gets a different id on the sequence depending of the order they are required on the user's code.
		//This ways a same type can get different ids when the user's code changes, but this queueId won't change
		//during the lifetime of the UnityMessager and MessageQueue instances. 
		int GetQueueId() { return m_queueId; }

		//Get the array id of the first UnityArray being used by the queue. The first array plays an important 
		//role on the C# side, allowing a message queue to reset itself without receiving special control messages
		int GetFirstArrayId() { return m_pFirstNode->unityArray.GetId(); }

		//Is reset means the current array is the first queue array and the current position is 0.
		bool IsReset();
		
		//Check IsReset comments. It is used to prepare the message queue to be used from its begining instead of
		//the previous current array and position. The start of the delivering process on the C# defines this moment,
		//since program execution is on the C# code and no other messages will be pushed until it is ended.
		virtual void Reset();

		//Used to free arrays that are not essential to keep the instance alive and functional. The first array is 
		//the unique that cannot be released, since the C# side stores it for reseting the queue by itself. 
		virtual void ReleaseArraysExceptFirst();

	protected:
		//Returns a pointer to the current next free position of the queue, while advancing the current position for 
		//the next call of AllocSpace. If the current array of the queue doesn't have space for the desired length,
		//the queue advances to its next array (allocating a new UnityArray if needed).
		T* AllocSpace(int length);

		//Advances the queue to its next UnityArray and sends a control message by accessing the UnityMessager instance
		//so the C# code will be warned before reading the next value when delivering the current message.
		void AdvanceToNextUnityArrayNode();

		//The node struct is used to just to stablishes a single linked list of allocated UnityArrays for the queue.
		typedef struct Node
		{
			UnityArray<T> unityArray;
			Node *pNext; //when pNext is NULL we reached the last allocated Node (and UnityArray) of the message queue.

			//The length will be always the same for a given MessageQueue
			Node(int length)
				: unityArray(), pNext(NULL) 
			{
				unityArray.Alloc(length);
			}

		} Node;

		Node* m_pFirstNode; //first node of the single linked list of nodes.
		Node* m_pCurrentNode; //current node (UnityArray) being filled
		int m_currentArrayPos; //next position to fill on the current node (UnityArray) of the queue.
		int m_queueId; //check comments for GetQueueId
	};

	//Control queue, always should be the message queue of id 0 on the UnityMessager message queues array
	//It is responsible for implementing the communication protocol between C++ and C#
	class ControlQueue : public MessageQueue<int>
	{
	public:
		//should be created together with the UnityMessager instance, by the constructor of MessageQueue<int>
		//it requires the singleton access point for UnityMessager to exist already when its being instanced.
		ControlQueue(); 

		//Returns false only if the queue can be considered empty, which is set by the C# code when all 
		//messages were delivered
		bool IsThereAnyMessageToDeliver();
		
		//Register an usual message to the queue, after that it is ready for registering parameters of the
		//last message sent or to start a new message sending by being re-called with another message.
		void SendMessage(int receiverId, int msgId);

		//Sends a control message, it means a message addressed to the UnityMessager receiver instance on C#.
		//These are special messages that can have only integer parameters and that can be sent while parameters 
		//for the last sent message still being filled, since they are essential for advancing a queue to 
		//its next array when the current array gets full
		void SendControlMessage(int msgId, int nOfParams, const int* intParams);
		
		//Registers a single parameter being pushed to the queue with the passed queueId. 
		void RegisterParam(int queueId);

		//Register an array parameter being pushed to the queue with the passed queueId.
		void RegisterParam(int queueId, int length);

	private:
		// *HIDES* the base AllocSpace method by an specific one that consider the need of keeping space for 
		// advancing the control queue itself to its next array before pushing new control values
		int* AllocSpace(int length);

		// *HIDES* the base correspoding method to avoid an endless recursion when the control queue needs to
		// advance itself to its next array.
		void AdvanceToNextUnityArrayNode();

		//points the control queue position where the number of parameters for the last sent message is 
		//registered in order to increment it when new parameters are pushed
		int *m_pCurrentNOfParams;
		
		//When set true AdvanceToNextUnityArrayNode should return, since it is already advancing by an enclosing call
		bool isAdvancingToNextNode;
	};

	//Parameter Queue, just one instance of it for each parameter type will exist.  
	template <typename T> 
	class ParamQueue : public MessageQueue<T>
	{
	public:
		//It is a sigleton that creates itself when accessed by the corresponding template method on UnityMessager. 
		static ParamQueue<T>& GetInstance() { return *(s_pInstance ? s_pInstance : (s_pInstance = new ParamQueue<T>())); }

		//It MUST be deleted in case the game is finished via the Unity Editor. In this case the singleton access point gets  
		//NULLed by the destructor being filled again by a new instance next time it is accessed, on a a new game run.
		virtual ~ParamQueue() { s_pInstance = NULL; }

		//push parameter method for single items
		void Push(const T& item);
		
		//push parameter method for arrays to be copied into the queue
		void Push(const T* pItemArray, int length);

		//push parameter method for arrays to be filled, returning the pointer to the first element to be filled. 
		void PushAndGetPtrToFill(T** const ppQueueSubArrayOutput, int length);

	protected:
		//declares the base method name as being template dependent to avoid compilation issues with emscripten
		using MessageQueue<T>::AllocSpace;

	private:
		ParamQueue() {}
		static ParamQueue<T>* s_pInstance; //singleton instance pointer holder
	};
};

}; //UnityForCpp

#include "UnityMessager.hpp"

#endif

