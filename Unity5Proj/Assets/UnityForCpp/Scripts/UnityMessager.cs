//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

using UnityEngine;
using UnityEngine.Assertions;
using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Reflection;

namespace UnityForCpp
{

public class UnityMessager : MonoBehaviour
{
    //Max number of simultaneous message receiver instances bound to the UnityMessager and so able to receive messages. Min value is 16.
    public int maxNumberOfReceiverIds = 16;
    
    //Max size in bytes of the memory blocks used by UnityMessager. Min value is 512, 1024 or 2048 are usually good values.
    public int maxQueueArraysSizeInBytes = 512;

    //SINGLETON access point 
    public static UnityMessager Instance { get { return _s_instance; } }

    //Provides an available receiver id, in such way you can bound your receiver instance (IMessageReceiver) to it using
    //SetReceiverObject. By requesting it from the C# code you should pass it to the C++ code so it can be used there.
    public int NewReceiverId()
    {
        int nextFreeId = _receiverIdsSharedArray[0]; //Position 0 holder the next free id
        if (nextFreeId == 0) //0 means there is no more available receive ids, THIS CAN NEVER HAPPEN, set a bigger value on the inspector
        {
            Debug.LogError("[UnityMessager] Not enough receiver ids set to your application!!");
            return -1;
        }

        //The receiver id shared array controls the free ids by implementing a single linked list where a given position
        //has the id of the next free position. So here we are removing the "nextFreeId" from this list by setting the head
        //of the list (position 0) to the position indicated by the nextFreeId position.
        _receiverIdsSharedArray[0] = _receiverIdsSharedArray[nextFreeId];
        _receiverIdsSharedArray[nextFreeId] = -1; //this indicates the id is not free, but also wasn't bound to a receiver yet.

        return nextFreeId;
    }

    //This method releases a receiver id setting null to the reference of the receiver instance bound to it.
    public void ReleaseReceiverId(int receiverId)
    {
        if (_receiverIdsSharedArray[receiverId] != -1) //this indicates the id is not in use, being in the free ids linked list.
        {
            Debug.LogWarning("[UnityMessager] Attempt to release a receiver id not being used!");
            return;
        }

        _receivers[receiverId] = null;
        _receiverGameObjects[receiverId] = null;

        //reinserts the id to the front of the single linked list, remembering the position 0 is always the head of the list.
        _receiverIdsSharedArray[receiverId] = _receiverIdsSharedArray[0];
        _receiverIdsSharedArray[0] = receiverId;
    }

    //Set a C# object as receiver (it may be any C# object, including a game object component if you want make it special, beyond the default)
    public void SetReceiverObject(int receiverId, IMessageReceiver receiver)
    {
        //Check this id was really delivered to you via NewReceiverId(), which may be called from C# or from C++
        if (_receiverIdsSharedArray[receiverId] != -1) 
        {
            Debug.LogError("[UnityMessager] Attempt to set an object to a 'free' Receiver id!. Use NewReceiverId for requesting an id before setting an object to it.");
            return;
        }

        _receivers[receiverId] = receiver;
    }

    //Set a game object as receiver and (optionally) specify the default receiver component
    //the default receiver component is the target for when SendMessage is used from C++ without any component specified
    public void SetReceiverObject(int receiverId, GameObject gameObject, IMessageReceiver defaultReceiverComponent = null)
    {
        //Check this id was really delivered to you via NewReceiverId(), which may be called from C# or from C++
        if (_receiverIdsSharedArray[receiverId] != -1)
        {
            Debug.LogError("[UnityMessager] Attempt to set an object to a 'free' Receiver id!. Use NewReceiverId for requesting an id before setting an object to it.");
            return;
        }

        _receiverGameObjects[receiverId] = gameObject;
        _receivers[receiverId] = defaultReceiverComponent != null? defaultReceiverComponent
                                        : gameObject.GetComponent(typeof(IMessageReceiver)) as IMessageReceiver;
    }

    //If true you usually should call DeliverMessagers right way. 
    public bool HasMessagesToDeliver
    {
        get { return _controlQueue.HasMessagesToBeDelivered; }
    }

    //Deliver ALL the received messages to the respective receivers. Handlers that call C++ code when handling messages MUST BE
    //SURE this C++ code will not send messages during the delivering process
    public void DeliverMessages()
    {
        if (!HasMessagesToDeliver)
            return;

        //This sets an end message so the delivering process doesn't continue forever and reset things on C++ side
        UnityMessagerDLL.UM_OnStartMessageDelivering();

        try
        {
            while (_controlQueue.HasMessagesToBeDelivered)
            {
                //_currentUniqueId purpose is only to make sure there was no forbidden calls to Message::FillUnityMessagerInternalMessageInstance
                Assert.IsTrue(++_currentUniqueId > 0); //always true, inside an assert just to be removed on release builds

                //The call bellow will read the next message to our internal instance (_internalMessageInstance), so it can be sent to receivers
                _controlQueue.ReadNextMessageToUnityMessagerInternalInstance();

                if (_internalMessageInstance.ComponentId >= 0) //is this message addressed to an specific game object component?
                {
                    GameObject receiverGameObject = null;
                    if (_internalMessageInstance.ReceiverId >= 0) //was the receiver specified by an id?
                        receiverGameObject = _receiverGameObjects[_internalMessageInstance.ReceiverId];
                    else
                    {   //else we have a name to look for with GameObject.Find
                        int objectNameLength = -_internalMessageInstance.ReceiverId;
                        string objectName = GetParamQueue<Byte>().ReadNextAsString(objectNameLength);
                        receiverGameObject = GameObject.Find(objectName);
                    }

                    if (receiverGameObject != null) //if we have found the receiver game object
                    {   
                        //Get the component based on its type, indexed by the component id
                        Component component = receiverGameObject.GetComponent(_componentTypes[_internalMessageInstance.ComponentId]);
                        if (component != null)
                        {
                            if (_internalMessageInstance.MessageId >= 0) //if this is a message to an IMessageReceiver component
                            {
                                IMessageReceiver receiverComponent = component as IMessageReceiver;
                                if (receiverComponent != null)
                                    receiverComponent.ReceiveMessage(ref _internalMessageInstance);
                                else
                                    Debug.LogError("[UnityMessager] Message sent to an invalid receiver component!");
                            }
                            else //else this message will be delivered via reflection
                                DeliverMessageWithReflection(component, ref _internalMessageInstance);
                        }
                        else
                            Debug.LogError("[UnityMessager] Message sent to an component the receiver game object doesn't have!!");
                    }
                    else
                        Debug.LogError("[UnityMessager] Message sent to an invalid or not found GameObject!!");
                }
                else //the message is addressed to a C# object (which may also be the default receiver component of game object receiver).
                {
                    IMessageReceiver receiver = _receivers[_internalMessageInstance.ReceiverId];
                    if (receiver != null)
                        receiver.ReceiveMessage(ref _internalMessageInstance);
                    else
                        Debug.LogError("[UnityMessager] Message sent to an invalid receiver object!!");
                }

                //If the message receiver has not read all the parameters we need to advance manually until getting to the start of the next message
                while (_controlQueue.NumberOfParamsBeforeNextMessage > 0)
                {
                    ParamInfo paramInfo = _controlQueue.CurrentParamInfo;
                    _messageQueues[paramInfo.QueueId].AdvanceToPos(paramInfo.ArrayLength >= 0 ? paramInfo.ArrayLength : 1);
                    _controlQueue.AdvanceToNextParam();
                }
            }
        }
        catch (Exception exception)
        {
            _controlQueue.OnFinishDeliveringMessages();
            throw exception;
        }
        finally
        {
            //Reset Message Queues so they get ready for next usage when delivering messages again.
            for (int i = 0; i < _messageQueues.Length; ++i)
                _messageQueues[i].Reset();
        }
    }

    //This method will release almost all the shared arrays (used blocks of memory having around "maxQueueArraysSizeInBytes" bytes each)
    //being currently used by UnityMessager. This is specially useful when your app loses focus and should free non essential stuff
    //If needed, the arrays released by this method will be allocated again in the future.
    public void ReleasePossibleQueueArrays()
    {
        UnityMessagerDLL.UM_ReleasePossibleQueueArrays();
    }

    //IMPLEMENT this interface in order to make your class objects capable of receiving and handling messages.
    public interface IMessageReceiver
    {
        void ReceiveMessage(ref UnityMessager.Message msg);
    }

    //All the data from a message is delivered to your receiver instance through an instance of this struct
    //YOU ARE NOT ALLOWED TO MAKE COPIES OF THIS OR TRYING TO KEEP THE REFERENCE BEYOND THE "ReceiveMessage" call.
    public struct Message
    {
        //The receiver associated with the receiving instance handling this message, usually you won't need it
        public int ReceiverId { get; private set; }

        //The component Id for the component this message is addressed, usually you won't need it
        public int ComponentId { get; private set; } 
        
        //Message Id is a value totally under your control, decide yourself the ids to represent your message types for a given
        //receiver type and use it on your ReceiveMessage method route the received message to the destination handler code.
        public int MessageId { get; private set; }
        
        //Total number of parameters sent together with this messager. Observe, however some of them may be already read and cannot
        //be read again. Check NumberOfParamsToBeRead for the parameters that still available for unpacking on the message queues.
        public int NumberOfParams { get; private set; }

        //Number of parameters on this message that were not read yet. Each time a method from Message with the "Advance" word on
        //its name is called, this value gets decremented, since the read parameter value cannot be read again.
        public int NumberOfParamsToBeRead
        {
            get
            {
                //safety to check to be sure this call is not coming from a copy of an old Message instance.
                if (_uniqueId < _s_currentUniqueId)
                    return 0;

                return UnityMessager.Instance._controlQueue.NumberOfParamsBeforeNextMessage;
            }
        }

        //Returns the next param type. If the next parameter is an array the element type of this array is returned.
        public Type NextParamType 
        {
            get
            {
                if (_paramInfo.QueueId < 0)
                    return null;

                return UnityMessager.Instance._messageQueues[_paramInfo.QueueId].QueueType;
            }
        }

        //Use this to check if the next parameter is an array of the type given by "NextParamType"
        public bool IsNextParamAnArray
        {
            get
            {
                if (_paramInfo.QueueId < 0)
                    return false;

                return _paramInfo.ArrayLength >= 0;
            }
        }

        //A parameter can be read as string if it has Byte as type and is an array. This method performs this check for you.
        public bool CanNextParamBeReadAsString
        {
            get
            {
                if (_paramInfo.QueueId < 0)
                    return false;

                return _paramInfo.ArrayLength >= 0
                        && UnityMessager.Instance._messageQueues[_paramInfo.QueueId].QueueType == typeof(Byte);
            }
        }

        //Reads the next parameter as a single value and advance to the next parameter if any. 
        public T ReadNextParamAndAdvance<T>()
        {
            Assert.IsTrue(CheckParamRequest(typeof(T), false)); //When assertions get compiled relevant additional checks are made          

            T param = UnityMessager.Instance.GetParamQueue<T>(_paramInfo.QueueId).ReadNext();
            Advance();
            return param;
        }

        //Reads the next parameter as an ArrayParam/T[] and advance to the next parameter if any. 
        //ArrayParam can be implicitly converted to T[], however by using it directly you may avoid the creation of a new array object,
        //accessing the values directly from the parameter queue. Check the ArrayParam class for more details.
        public ArrayParam<T> ReadNextParamAsArrayAndAdvance<T>()
        {
            Assert.IsTrue(CheckParamRequest(typeof(T), true)); //When assertions get compiled relevant additional checks are made          

            ArrayParam<T> param = UnityMessager.Instance.GetParamQueue<T>(_paramInfo.QueueId).ReadNextAsArray(_paramInfo.ArrayLength);
            Advance();
            return param;
        }

        //Reads the next parameter as an string and advance to the next parameter if any.
        public string ReadNextParamAsStringAndAdvance()
        {
            Assert.IsTrue(CheckParamRequest(typeof(Byte), true)); //When assertions get compiled relevant additional checks are made          

            string param = UnityMessager.Instance.GetParamQueue<Byte>(_paramInfo.QueueId).ReadNextAsString(_paramInfo.ArrayLength);
            Advance();
            return param;
        }

        //Reads the next parameter (single or array) as an object and advance to the next parameter if any. 
        //if it is an array, the object will be a c# native array of the current parameter type (T[])
        public object ReadNextParamAndAdvance()
        {
            bool isArray = IsNextParamAnArray;
            Assert.IsTrue(CheckParamRequest(null, isArray)); //When assertions get compiled relevant additional checks are made          

            object param = isArray ?
                            UnityMessager.Instance._messageQueues[_paramInfo.QueueId].ReadNextAsArrayBase(_paramInfo.ArrayLength)
                            : UnityMessager.Instance._messageQueues[_paramInfo.QueueId].ReadNextAsObject();
            Advance();
            return param;
        }

        //Skip the next parameter and advance to the next parameter after that if any.
        public void SkipNextParamAndAdvance()
        {
            UnityMessager.Instance._controlQueue.AdvanceToNextParam();
            _paramInfo = UnityMessager.Instance._controlQueue.CurrentParamInfo;
        }

        //FOR INTERNAL USAGE ONLY!! This method fills the data from the next message to be handled directly into
        //the internal Message instance of the UnityMessager, avoiding the need of exposing a public constructor or factory method
        public static void FillUnityMessagerInternalMessageInstance(int rcvId, int rtnId, int msgId, int nOfParams) 
        {
            UnityMessager unityMessager = UnityMessager.Instance;
            
            //Check there was no forbidden calls made to this method
            Assert.IsTrue(_s_currentUniqueId < unityMessager._currentUniqueId,
                          "[UnityMessager] NOT ALLOWED CALL TO FillUnityMessagerInternalMessageInstance, RESTART THE GAME!!");

            unityMessager._internalMessageInstance.ReceiverId = rcvId;
            unityMessager._internalMessageInstance.ComponentId = rtnId;
            unityMessager._internalMessageInstance.MessageId = msgId;
            unityMessager._internalMessageInstance.NumberOfParams = nOfParams;

            //gives this message an uniqueId in such way it will not be able to request parameters anymore 
            //after the ReceiveMessage method has advanced to the next message
            unityMessager._internalMessageInstance._uniqueId = ++_s_currentUniqueId;
            //reads the current parameter to be delivered as "NextParam" on the methods of this struct
            unityMessager._internalMessageInstance._paramInfo = UnityMessager.Instance._controlQueue.CurrentParamInfo;
        }

        //Advance to the next parameter (actually the one that comes after the one said as "next" on method titles
        private void Advance()
        {
            //Now we are advancing to the parameter the comes after the one we calling "Next Parameter" by this method name.
            UnityMessager.Instance._controlQueue.AdvanceToNextParam();
            //so now we have it ready to the next request of -1 set to the _paramInfo.QueueId if there is no other params for this message
            _paramInfo = UnityMessager.Instance._controlQueue.CurrentParamInfo;
        }

        //Perform additional checks to log errors to the user when parameters are being requested in a wrong way for this message instance
        private bool CheckParamRequest(Type requestedType, bool wasRequestedAsArray)
        {
            if (_uniqueId < _s_currentUniqueId) 
            {
                Debug.LogError("[UnityMessager] ReadNextParamAndAdvance called for an old Message which parameters were already read.");
                return false;
            }

            if (_paramInfo.QueueId < 0)
            {
                Debug.LogError("[UnityMessager] Attempt to read a parameter beyond the ones of available for the message!");
                return false;
            }

            if (requestedType != null && UnityMessager.Instance._messageQueues[_paramInfo.QueueId].QueueType != requestedType)
            {
                Debug.LogError("[UnityMessager] Parameter of wrong type being requested!");
                return false;
            }

            if ((_paramInfo.ArrayLength >= 0) != wasRequestedAsArray)
            {
                if (wasRequestedAsArray)
                    Debug.LogError("[UnityMessager] Single parameter value requested as array!");
                else
                    Debug.LogError("[UnityMessager] Array parameter requested as single value!");

                return false;
            }

            return true;
        }

        private ParamInfo _paramInfo;
        private ulong _uniqueId;

        private static ulong _s_currentUniqueId = 0; //gets increamented at each new message, providing always an unique id.
    };

    //Use this class for receiving and handling array parameters. It usually avoids the need of creating a C# native array instance,
    //if you need it however, you may use "CopyTo" to fill your C# native array.
    public struct ArrayParam<T>
    {
        public int Length { get; private set; }

        public T this[int i]
        {
            get 
            {
                Assert.IsTrue(i >= 0 && i < Length);
                return _array[_firstIdx + i]; 
            }
        }

        public void CopyTo(T[] destArray, int destStartIdx, int thisStartIdx, int copyLength)
        {
            Assert.IsTrue(thisStartIdx >= 0 && (thisStartIdx + copyLength) <= Length);
            Array.Copy(_array, _firstIdx + thisStartIdx, destArray, destStartIdx, copyLength);
        }

        //better to avoid this implicit conversion on performance sensitive code.
        public static implicit operator T[](ArrayParam<T> array)
        {
            T[] result = new T[array.Length];
            array.CopyTo(result, 0, 0, array.Length);
            return result;
        }

        public ArrayParam(T[] array, int firstIdx, int length)
            : this()
        {
            _array = array;
            _firstIdx = firstIdx;
            Length = length;
        }

        private T[] _array;
        private int _firstIdx;
    }

    //This is only used on debug code to make sure no invalid calls to FillUnityMessagerInternalMessageInstance are being made
    private ulong _currentUniqueId = 0;

    //Point of return for the method Message.FillUnityMessagerInternalMessageInstance. Once it is filled it is delivered to its receiver.
    private Message _internalMessageInstance;

    //shared array for controling the available receiver ids, keep them in synch between the C++ code and the C# code.
    private int[] _receiverIdsSharedArray = null;

    //message receiver instances (OR default component instances) accordingly to their receiverIds. 
    private IMessageReceiver[] _receivers = null;

    //message receiver game object instances accordingly to their receiverIds
    private GameObject[] _receiverGameObjects = null;

    //component types, this is indexed by the unique internal component id for the component type
    private List<Type> _componentTypes = null;

    //Message queues arrays, where the 0 position is the control queue and the next ones are the ParamQueue instances, one per param type
    private MessageQueueBase[] _messageQueues = null;
    private ControlQueue _controlQueue = null;

    private const int _controlQueueId = 0; //MUST BE 0, corresponds to UM_CONTROL_QUEUE_ID on C++
    private const int _maxNOfMessageQueues = 32; //corresponds to UM_MAX_N_OF_MESSAGE_QUEUES on C++, check comments for this. 

    private static UnityMessager _s_instance = null; //singleton reference holder

#if UNITY_EDITOR
    //Check min value requirement are met for the inspector settings for this class 
    void OnValidate()
    {
        if (maxNumberOfReceiverIds < 16)
        {
            Debug.LogWarning("Max Number Of Receiver Ids cannot be less than 16!!");
            maxNumberOfReceiverIds = 16;
        }

        if (maxQueueArraysSizeInBytes < 512)
        {
            Debug.LogWarning("Max Queue Arrays' Size In Bytes Cannot Be Less Than 512!!");
            maxQueueArraysSizeInBytes = 512;
        }
    }
#endif

    private void Awake()
    {
        if (_s_instance != null && _s_instance != this)
        {
            Destroy(this.gameObject);
            throw new Exception("[UnityMessager] More than one instance of UnityMessager being created!!");
        }
        
        if (_s_instance == null)
        {
            _s_instance = this;
            DontDestroyOnLoad(this.gameObject);
        }

        int firstArrayId = UnityMessagerDLL.UM_InitUnityMessagerAndGetControlQueueId(maxNumberOfReceiverIds, maxQueueArraysSizeInBytes);
        _controlQueue = new ControlQueue(firstArrayId);

        _messageQueues = new MessageQueueBase[_maxNOfMessageQueues];
        _messageQueues[0] = _controlQueue;

        //We create instance of MessageQueueBase so they can hold received ids for the first and current queue arrays
        //until the user's code requests the corresponding parameter using a generic method for the first time, what will
        //cause the real ParamQueue to be instanced and replace this placeholder instance we set here.
        for (int i = 1; i < _messageQueues.Length; ++i)
            _messageQueues[i] = new MessageQueueBase(i);

        _receivers = new IMessageReceiver[maxNumberOfReceiverIds];
        //Internal MessageReceiver class, handle messages sent to the UnityMessager itself. The receiver Id 0 is known as being its receiver id by default.
        _receivers[0] = new MessageReceiver();

        _receiverGameObjects = new GameObject[maxNumberOfReceiverIds];

        _componentTypes = new List<Type>();

        DeliverMessages(); //Deliver the first messages, expected to be only control messages to finish with the intialization process
    }

    private void OnDestroy()
    {
        //This will delete the singleton instance on C++ and release all the shared arrays allocated by the UnityMessager
        UnityMessagerDLL.UM_OnDestroy();
    }

    //Registers a new component type being used by the C++ code as receiver component for game object receivers
    private void RegisterComponentType(int id, string typeName)
    {
        Assert.IsTrue(_componentTypes.Count == id);//the id is the index, it would be strange if this assertion fails

        Type componentType = Type.GetType(typeName);
        Assert.IsTrue(componentType != null, "[UnityMessager] Invalid component type being used from C++");

        _componentTypes.Add(componentType);
    }

    //helper method for getting the Parameter Queue created at the first time its accessed for an specific type
    private ParamQueue<T> GetParamQueue<T>(int queueId = -1) 
    {
        //If this is the first time we are reading a parameter of the type T we need to create the ParamQueue for it.
        if (ParamQueue<T>.Instance == null)
        {
            if (queueId < 0)
            {
                for (queueId = 0; queueId < _maxNOfMessageQueues; ++queueId)
                {
                    if (_messageQueues[queueId].QueueType == typeof(T))
                        break;
                }

                if (queueId == _maxNOfMessageQueues)
                    return null;
            }
            //We do that using the existing instance to MessageQueueBase for the respective queueId.
            //This base class instance has held for us the ids provided in the past for this queueId. 
            ParamQueue<T>.CreateInstance(UnityMessager.Instance._messageQueues[queueId]);
            //Saves the new ParamQueue instance replacing the base class instance we don't need anymore
            _messageQueues[queueId] = ParamQueue<T>.Instance;
        }

        return ParamQueue<T>.Instance;
    }

    //helper method to get a message based on reflection delivered with the message parameters extracted
    private void DeliverMessageWithReflection(object component, ref Message message)
    {
        int methodNameLength = -message.MessageId; //messageId, when negative, is the length of the method name string
        string methodName = GetParamQueue<Byte>().ReadNextAsString(methodNameLength);

        MethodInfo methodInfo = component.GetType().GetMethod(methodName);

        ParameterInfo[] paramInfos = methodInfo.GetParameters();

        Assert.IsTrue(message.NumberOfParams == paramInfos.Length);
        object[] parameters = null;

        if (paramInfos.Length > 0) //is there any parameter to read
        {
            parameters = new object[paramInfos.Length];
            int i = -1;
            while (message.NumberOfParamsToBeRead > 0)
                parameters[++i] = paramInfos[i].ParameterType == typeof(string) ? //read a string as a string, so we don't get a byte array
                                  message.ReadNextParamAsStringAndAdvance()
                                  : message.ReadNextParamAndAdvance();
        }

        methodInfo.Invoke(component, parameters);
    }

    //Control messages are specific to setup or control the message delivering process itself  
    private void HandleControlMessage(int messageId, ArrayParam<int> arrayParam)
    {
        switch (messageId)
        {
            case 0: //UMM_SET_QUEUE_ARRAY = 0, used when the current array in use is close to its end, so a new one is needed
                {   
                    Assert.IsTrue(arrayParam.Length == 2);

                    int queueId = arrayParam[0];
                    int arrayId = arrayParam[1];

                    _messageQueues[queueId].SetCurrentArray(arrayId);
                    break;
                }
            case 1: //UMM_SET_QUEUE_FIRST_ARRAY = 1, first array of a given queue, we keep this info forever so we can reset queues
                {
                    Assert.IsTrue(arrayParam.Length == 2);

                    int queueId = arrayParam[0];
                    int arrayId = arrayParam[1];
                    
                    _messageQueues[queueId].SetFirstArray(arrayId);
                    break;
                }
            case 2: //UMM_SET_RECEIVER_IDS_ARRAY = 2, sets the id for the control array of available receiver ids
                {
                    Assert.IsTrue(arrayParam.Length == 1);

                    int arrayId = arrayParam[0];

                    _receiverIdsSharedArray = UnityAdapter.Instance.GetSharedArray<int>(arrayId);
                    break;
                }
            default:
                Debug.LogError("[UnityMessager] Unknow message id received by UnityMessager.HandleControlMessage!");
                break;
        }
    }

    //Internal message receiver class, to handle messages addressed to UnityMessager itself, not being control messages
    private class MessageReceiver : IMessageReceiver
    {
        public void ReceiveMessage(ref UnityMessager.Message msg)
        {
            switch (msg.MessageId)
            {
                case 3: //UMM_FINISH_DELIVERING_MESSAGES = 3, Send to mark the last message to be delivered (THIS ONE). 
                    UnityMessager.Instance._controlQueue.OnFinishDeliveringMessages();
                    break;
                case 4: //UMM_REGISTER_NEW_COMPONENT = 4
                    UnityMessager.Instance.RegisterComponentType(msg.ReadNextParamAndAdvance<int>(), 
                                                                 msg.ReadNextParamAsStringAndAdvance());
                    break;
                default:
                    Debug.LogError("[UnityMessager] Unknow message id received by UnityMessager.ReceiveMessage!");
                    break;
            }
        }
    }

    private struct ParamInfo
    {
        //Parameter queue id, -1 if it represents that there is no more parameters to be read for a given Message.
        public int QueueId { get; private set; }

        //Array length if the parameter is an array, -1 if it is a single value.
        public int ArrayLength { get; private set; }

        public ParamInfo(int queueId, int arrayLength) 
            : this()
        {
            QueueId = queueId;
            ArrayLength = arrayLength;
        }
    }

    private class MessageQueueBase 
    {
        //Corresponds to the QueueId on the C++ code and to the index of the instance on the UnityMessager._messageQueues array.
        public int QueueId { get; private set; }

        //ParamQueue<T> parameter (T) type, null if this is not a ParamQueue instance. 
        public Type QueueType { get; private set; }

        //Usual contructor
        public MessageQueueBase(int queueId)
        {
            QueueId = queueId;
            QueueType = null;
        }

        //Special constructor used by ParamQueue on its constructing process, check this for more comments 
        public MessageQueueBase(MessageQueueBase instanceToCopy)
        {
            QueueId = instanceToCopy.QueueId;
            QueueType = instanceToCopy.QueueType;

            _firstArrayBase = instanceToCopy._firstArrayBase;
            _currentArrayBase = instanceToCopy._currentArrayBase;
            _currentArrayPos = instanceToCopy._currentArrayPos;
        }

        //Sets the id of the first array for this queue. Having this info we can reset the queue to its first id after delivering all the messages
        public void SetFirstArray(int arrayId) 
        {
            _firstArrayBase = UnityAdapter.Instance.GetSharedArray(arrayId);
            _currentArrayBase = _firstArrayBase;
            QueueType = _firstArrayBase.GetType().GetElementType();
        }

        //Just updates the current array id on this base class method. Derived classes should update their array references also.
        public virtual void SetCurrentArray(int arrayId) 
        {
            _currentArrayBase = UnityAdapter.Instance.GetSharedArray(arrayId);
            _currentArrayPos = 0;
        }

        //Resets the Message queue so it starts from its beginning at the next Message delivering process
        public virtual void Reset()
        {
            _currentArrayBase = _firstArrayBase;
            _currentArrayPos = 0;
        }

        //Advance the desired number of items on this queue without actually reading the held values.
        //Useful for when the message receiver has returned without reading all the method parameters.
        public void AdvanceToPos(int nOfItemsToSkip)
        {
            _currentArrayPos += nOfItemsToSkip;
        }

        //Read the next value on the queue as a single param value
        public object ReadNextAsObject()
        {
            return _currentArrayBase.GetValue(_currentArrayPos++);
        }

        //Read the next value on the queue as an array value
        public Array ReadNextAsArrayBase(int length)
        {
            var array = Array.CreateInstance(QueueType, length);
            Array.Copy(_currentArrayBase, _currentArrayPos, array, 0, length);
            _currentArrayPos += length;

            return array;
        }

        protected int _currentArrayPos = 0;
        protected Array _firstArrayBase = null;
        protected Array _currentArrayBase = null;
    }

    private abstract class MessageQueue<T>: MessageQueueBase
    {
        public MessageQueue(int queueId, int firstArrayId)
            : base(queueId)
        {
            SetFirstArray(firstArrayId);
            _firstArray = _currentArray = _firstArrayBase as T[];
        }

        //complements the base class version by actually keeping references to the first and current queue arrays 
        public MessageQueue(MessageQueueBase baseInst)
            : base(baseInst)
        {
            _firstArray = _firstArrayBase as T[];
            _currentArray = _firstArrayBase == _currentArrayBase ? _firstArray : _currentArrayBase as T[];
        }

        //complements the base class version by actually updating the current array reference 
        public override void SetCurrentArray(int arrayId)
        {
            base.SetCurrentArray(arrayId);
            _currentArray = _currentArrayBase as T[];
        }

        //complements the base class version by actually updating the current array reference 
        public override void Reset()
        {
            base.Reset();
            _currentArray = _firstArray;
        }

        protected T[] _firstArray = null;
        protected T[] _currentArray = null;
    }

    private class ControlQueue: MessageQueue<int>
    {
        public ControlQueue(int firstArrayid): base(UnityMessager._controlQueueId, firstArrayid) 
        {
            CurrentParamInfo = new ParamInfo(-1, 0);
        }

        public bool HasMessagesToBeDelivered { get { return _firstArray[0] != _emptyCode; } }

        //This includes the current parameter, for which info is already read to the CurrentParamInfo property
        public int NumberOfParamsBeforeNextMessage { get; private set; }

        //This advances the control queue over the next message, creating the Message instance to be handled and preparing for 
        //reading the parameters (actually the first parameter info is already read here by default).
        public void ReadNextMessageToUnityMessagerInternalInstance() 
        {  
            Assert.IsTrue(NumberOfParamsBeforeNextMessage == 0);
            DeliverControlMessagesIfAny(); //very important since queue arrays may be changed by these messages

            int receiverId = _currentArray[_currentArrayPos + 0];
            int messageId = _currentArray[_currentArrayPos + 1];
            NumberOfParamsBeforeNextMessage = _currentArray[_currentArrayPos + 2];
            int componentId = -1;

            if (NumberOfParamsBeforeNextMessage >= 0) //normal C# object receiver
                _currentArrayPos += 3;                
            else
            {   //A negative number of parameters indicates this message is addressed to a component from a game object receiver
                //observe control messages are an exception to this rule, we know they are control messages because receiverId == 0
                componentId = _currentArray[_currentArrayPos + 3];
                _currentArrayPos += 4;
                NumberOfParamsBeforeNextMessage = - NumberOfParamsBeforeNextMessage - 1;
            }

            ReadCurrentParam();


            Message.FillUnityMessagerInternalMessageInstance(receiverId, componentId, messageId,
                                                              NumberOfParamsBeforeNextMessage);
        }

        //The current parameter info is actually considered to be the "next parameter info" by the Message struct
        public ParamInfo CurrentParamInfo { get; private set; }

        public void AdvanceToNextParam()
        {
            --NumberOfParamsBeforeNextMessage;
            ReadCurrentParam();
        }

        //So HasMessagesToBeDelivered returns false and ends the message delivering loop
        public void OnFinishDeliveringMessages() { _firstArray[0] = _emptyCode; }

        //Reads the current parameter info to the CurrentParamInfo property
        private void ReadCurrentParam()
        {
            if (NumberOfParamsBeforeNextMessage <= 0)
            {   //no more parameters to read
                CurrentParamInfo = new ParamInfo(-1, 0);
                return;
            }

            DeliverControlMessagesIfAny(); //very important since queue arrays may be changed by these messages
            
            Assert.IsTrue(_currentArray[_currentArrayPos] != 0);

            if (_currentArray[_currentArrayPos] > 0) //is a single value parameter
            {
                CurrentParamInfo = new ParamInfo(_currentArray[_currentArrayPos], -1);
                ++_currentArrayPos;
            }
            else // is an array parameter (queue id set as negative as a mark it is an array param, so size is at the next position)
            {
                CurrentParamInfo = new ParamInfo(- _currentArray[_currentArrayPos], _currentArray[_currentArrayPos + 1]);
                _currentArrayPos += 2;
            }
        }

        //Check if the next item on the control queue is a control message and deliver it if that is the case
        //more than one control message may come in sequence before the next non control message or the next parameter
        private void DeliverControlMessagesIfAny()
        {
            //Control messages are marked by having 0 as receiverId (UnityMessager itself as receiver) and by
            //having the number of parameters set as negative.
            while (_currentArray[_currentArrayPos] == 0 && _currentArray[_currentArrayPos + 2] < 0)
            {
                int msgId = _currentArray[_currentArrayPos + 1];
                int nOfParams = -_currentArray[_currentArrayPos + 2];
                int firstIdx = _currentArrayPos + 3;

                _currentArrayPos += (3 + nOfParams);

                //only integer parameters are supported on control messages, all passed directly on the control queue
                UnityMessager.Instance.HandleControlMessage(msgId, new ArrayParam<int>(_currentArray, firstIdx, nOfParams));                                                 
            }
        }
        //code to mark all the messages were delivered, UM_EMPTY_CONTROL_QUEUE_CODE is the corresponding constant at C++
        private const int _emptyCode = -123456;
    }

	private class ParamQueue<T> : MessageQueue<T>
	{
        //Singleton access point.
        public static ParamQueue<T> Instance { get; private set; }

        //Creates the singleton instance based on the base instance, which exists only for holding first and current id
        //values and also the current position while real ParamQueue was not instanced by the generic parameter reader methods. 
        public static void CreateInstance(MessageQueueBase baseInst)
        {
            Assert.IsTrue(Instance == null, "[UnityMessager] More than one ParamQueue instance being created for a same type!");
            Instance = new ParamQueue<T>(baseInst);
        }

        //Read the next value on the queue as a single param value
        public T ReadNext()
        {
            return _currentArray[_currentArrayPos++];
        }

        //Read the next value on the queue as an array value
        public ArrayParam<T> ReadNextAsArray(int length)
        {
            _currentArrayPos += length;
            return new ArrayParam<T>(_currentArray, _currentArrayPos - length, length);
        }

        //Read the next value on the queue as an string. ONLY VALID FOR THE Byte ParamQueue!!!
        public string ReadNextAsString(int length)
        {
            _currentArrayPos += length;
            return System.Text.ASCIIEncoding.ASCII.GetString(_currentArray as byte[], _currentArrayPos - length, length);           
        }

        private ParamQueue(MessageQueueBase baseInst) : base(baseInst) {}
    }

#if (UNITY_WEBGL || UNITY_IOS) && !(UNITY_EDITOR)
    const string DLL_NAME = "__Internal";
#else
    const string DLL_NAME = "UnityForCpp";
#endif

    //Expose C++ DLL methods related to the UnityMessager C++ code, check comments on that for more details on these methods
    private class UnityMessagerDLL
    {
        [DllImport(DLL_NAME)]
        public static extern int UM_InitUnityMessagerAndGetControlQueueId(int maxNOfReceiverIds, int maxQueueArraysSizeInBytes);

        [DllImport(DLL_NAME)]
        public static extern void UM_OnStartMessageDelivering();

        [DllImport(DLL_NAME)]
        public static extern void UM_ReleasePossibleQueueArrays();

        [DllImport(DLL_NAME)]
        public static extern void UM_OnDestroy();
    }
}
}
