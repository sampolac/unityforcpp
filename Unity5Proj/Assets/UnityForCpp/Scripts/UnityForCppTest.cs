//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

using UnityEngine;
using UnityEngine.Assertions;
using System.Collections;
using System;
using System.Runtime.InteropServices;
using UnityForCpp;

//This class works together with the TestPlugin.cpp file in order to test the features provided by UnityForCpp
public class UnityForCppTest : MonoBehaviour, UnityMessager.IMessageReceiver {

    //game object to have its position, scale, rotation and color manipulated from the C++ code as a test
    public GameObject testObjectPrefab;
    //number of game object instances being managed from the C++ code
    public int numberOfGameObjects = 100;
    //interval between a serie of random updates to game objects made through the UnityMessager in order to test it.
    public float randomUpdatesInterval = 0.064f;

    //SINGLETON access point 
    public static UnityForCppTest Instance { get { return _s_instance; } }

    //Just for a quick test, by calling it from cpp with SendMessage<UnityForCppTest>("UnityForCppTest", "TestReflectionBasedMessage", ...)
    public void TestReflectionBasedMessage(int intValue, float floatValue, int[] intArray, string strValue, double doubleValue)
    {
        Debug.Log("[UnityForCppTest] TestReflectionBasedMessage called!");
    }

    //------------ START OF THE EXPOSED C# INTERFACE -------------------------
    public void SetGameObjectRotation(int gameObjectId, float rotation)
    {
        _gameObjectTransfoms[gameObjectId].Rotate(0.0f, 0.0f, rotation);
    }

    public void SetGameObjectColor(int gameObjectId, float r, float g, float b)
    {
        _gameObjectTransfoms[gameObjectId].gameObject.GetComponent<SpriteRenderer>().color = new Color(r, g, b);
    }

    //This is more to test array parameters while allowing overloaded signatures at the C++ side for a same message id
    private void SetGameObjectColor(int gameObjectId, UnityMessager.ArrayParam<float> colorComponents)
    {
        Assert.IsTrue(colorComponents.Length == 3);
        SetGameObjectColor(gameObjectId, colorComponents[0], colorComponents[1], colorComponents[2]);
    }

    //Called from C++ through a message only to the initalization process of this singleton UnityForCppTest C# instance
    //this informs the shared array we should look into at each Update() call in order to update the position of each game object
    private void SetPositionsArray(int arrayId)
    {
        _gameObjectPositions = UnityAdapter.Instance.GetSharedArray<Vec2>(arrayId);
    }

    //This method is a mere representation of C# method you would use to create instances following order from the C++ code.
    //however it is a poor example since it doesn't receive any parameter within the message itself. If your created C# object
    //were meant to be also a IMessageReceiver object, it should receive at least the receiver id issued from the C++ code as parameter.
    private void InstanceGameObject(int gameObjectId, int receiverId)
    {
        var startPos = ScreenCoordsFromCppCoords(_gameObjectPositions[gameObjectId]);
        var gameObject = Instantiate(testObjectPrefab, startPos, Quaternion.identity) as GameObject;
        _gameObjectTransfoms[gameObjectId] = gameObject.GetComponent<Transform>();

        if (receiverId > 0)
            UnityMessager.Instance.SetReceiverObject(receiverId, gameObject);
    }

    //Test method for receiving and using an string parameter
    private void DebugLogMessage(string strToLog, int logType = 0)
    {
        switch (logType)
        {
            case 0:
                Debug.Log("[UnityForCppTest LogMsg] " + strToLog);
                break;
            case 1:
                Debug.LogWarning("[UnityForCppTest LogMsg] " + strToLog);
                break;
            case 2:
                Debug.LogError("[UnityForCppTest LogMsg] " + strToLog);
                break;
        }
    }

    //Test method for receiving any crazy parameter combination and printing a list with their types
    private void LogParameterTypes(UnityMessager.Message message)
    {
        System.Text.StringBuilder logString = new System.Text.StringBuilder("Test Params message received with parameters: ");

        while (message.NumberOfParamsToBeRead > 0)
        {
            if (message.CanNextParamBeReadAsString)
                logString.Append("string, ");
            else
            {
                logString.Append(message.NextParamType.Name);
                if (message.IsNextParamAnArray)
                    logString.Append(" Array");

                logString.Append(", ");
            }

            message.SkipNextParamAndAdvance();
        }

        logString.Remove(logString.Length - 2, 2);

        Debug.Log(logString);
    }
    //------------ END OF THE EXPOSED C# INTERFACE -------------------------

    //C# version of this structure supported also at the C++ side
    public struct Vec2
    {
        public float x;
        public float y;

        public static implicit operator Vector3(Vec2 vec2)
        {
            return new Vector3(vec2.x, vec2.y, 0.0f);
        }
    }

    //reference to the game object transform components instead of the game objects themselves, just because
    //we will be accessing the transform much more than any other component, and also because it is easy to retrieve
    //the game object by using the "gameObject" field.
    private Transform[] _gameObjectTransfoms = null;

    //shared array with the object positions. Coordinates vary from -1 to 1, being -1 the left, botton of the screen and 1 the right, top
    private Vec2[] _gameObjectPositions = null;

    //holder to the singleton instance reference
    private static UnityForCppTest _s_instance = null;
 
    private void Awake()
    {
        if (_s_instance != null && _s_instance != this)
        {
            Destroy(this.gameObject);
            throw new Exception("[UnityForCppTest] More than one instance of UnityAdapter being created!!");
        }
        else
        {
            _s_instance = this;
            DontDestroyOnLoad(this.gameObject);
        }

        _gameObjectTransfoms = new Transform[numberOfGameObjects];
    }

	private void Start() 
    {
        Debug.Log("[UnityForCppTest] Start the UnityForCpp plugin tests");

        var camera = GameObject.Find("Main Camera").GetComponent<Camera>();
        Assert.IsTrue(camera.orthographic, "[UnityForCppTest] Camera MUST be orthographic!!");

        //Since it is meant to be a quick test case for the UnityForCpp features we avoiding using a Pixel Perfect camera script
        //and make here a very rough implementation of it by the calculus bellow
        float pixelsToUnits = 1f;
        camera.orthographicSize = (Screen.height / 2.0f) / pixelsToUnits;

        //Requests a new received id to be informed to the C++ code. This operation is suitable to be done here since this receiver
        //object is a singleton that will exist, if it was created by an order from the C++ code the receiver id should be requested there.
        int thisId = UnityMessager.Instance.NewReceiverId();

        //bounds the MessagerReceiver internal instance to the receiver id given above.
        UnityMessager.Instance.SetReceiverObject(thisId, this.gameObject, this);

        //T_InitTest Performs the first UnityForCpp tests and initialize the test case related to this C# class 
        TestDLL.T_InitTest(thisId, numberOfGameObjects, randomUpdatesInterval);

        //Deliver he messages we expect from the call above to the C++ code, they should finish the initialization process of this class
        UnityMessager.Instance.DeliverMessages();
    }

    private void Update()
    {
        //Call the C++ code to update the logic side of our test case there, so we can render its "view" side here
        TestDLL.T_UpdateTest(Time.deltaTime);

        //Since we know not only messages were delivered, but also the position of the objects were updated on the shared array
        //we opt by updating the positions first, so the messages can rely the target objects are already at their updated position.
        UpdateGameObjectPositions();

        //Deliver messages from the C++ code, you get no messages sometimes depending on the value set to randomUpdatesInterval 
        UnityMessager.Instance.DeliverMessages();
    }

    //Here we simulate a situation where it is interesting to release all the non essential memory we have allocated
    private void OnApplicationPause(bool pauseStatus)
    {
        if (pauseStatus)
            UnityMessager.Instance.ReleasePossibleQueueArrays();
    }

    private void OnDestroy()
    {
        Debug.Log("[UnityForCppTest] Ending the test.");
        TestDLL.T_TerminateTest();
    }

    //just go over the shared positions array updating the position for each object
    private void UpdateGameObjectPositions()
    {
        for (int i = 0; i < numberOfGameObjects; ++i)
            _gameObjectTransfoms[i].position = ScreenCoordsFromCppCoords(_gameObjectPositions[i]);
    }

    //For simplicity of this test case the Cpp coords considers the screen coords going from -1 to 1. 
    //So here we translate that to the real screen coordinates, what should match the final position of our 
    //"pixel perfect" based camera.
    private static Vector3 ScreenCoordsFromCppCoords(Vec2 vec2)
    {
        return new Vector3(0.5f * Screen.width * vec2.x, 0.5f * Screen.height * vec2.y, 0.0f);
    }

    //Here we "unpack" the message routing it to the destination method by the message id value
    public void ReceiveMessage(ref UnityMessager.Message msg)
    {
        switch (msg.MessageId)
        {
            case 0: //TRM_SET_GAME_OBJECT_ROTATION = 0,
                {
                    //this is commented out because we consider there is not need to check if the number of parameters is correct
                    //when the methods at the C++ side are wrapped by a C++ method signature, which assure paramaters are 
                    //correct in number and types. If we were using UNITY_MESSAGER.SendMessage directly on our game logic on C++
                    //so it could make sense to make sure at least the expected number of parameters are correct
                    //Assert.IsTrue(msg.NumberOfParamsToBeRead == 1);
                        
                    UnityForCppTest.Instance.
                        SetGameObjectRotation(msg.ReadNextParamAndAdvance<int>(), //gameObjectId
                                                msg.ReadNextParamAndAdvance<float>()); //rotation

                    break;
                }

            case 1: //TRM_SET_GAME_OBJECT_COLOR = 1
                {
                    int gameObjectId = msg.ReadNextParamAndAdvance<int>();
                    if (msg.NumberOfParamsToBeRead > 1)
                        UnityForCppTest.Instance.
                            SetGameObjectColor(gameObjectId, //gameObjectId
                                            msg.ReadNextParamAndAdvance<float>(), //r
                                            msg.ReadNextParamAndAdvance<float>(), //g
                                            msg.ReadNextParamAndAdvance<float>()); //b
                    else
                        UnityForCppTest.Instance.
                            SetGameObjectColor(gameObjectId, msg.ReadNextParamAsArrayAndAdvance<float>());

                    break;
                }
            case 2: //TRM_SET_POSITIONS_ARRAY = 2,
                {
                    UnityForCppTest.Instance.SetPositionsArray(msg.ReadNextParamAndAdvance<int>()); //arrayId
                    break;
                }
            case 3: //TRM_INSTANCE_GAME_OBJECT = 3,
                {
                    UnityForCppTest.Instance.InstanceGameObject(msg.ReadNextParamAndAdvance<int>(),
                                                                msg.ReadNextParamAndAdvance<int>()); //gameObjectId
                    break;
                }
            case 4: //TRM_DEBUG_LOG_MESSAGE = 4
                {
                    UnityForCppTest.Instance.DebugLogMessage(msg.ReadNextParamAsStringAndAdvance(),
                                                                msg.ReadNextParamAndAdvance<int>());                        
                    break;
                }
            case 5: //TRM_LOG_PARAM_TYPES = 5
                {
                    UnityForCppTest.Instance.LogParameterTypes(msg);
                    break;
                }
            default:
                Debug.LogError("[UnityForCppTest] Unknow message id received by UnityForCppTest.ReceiveMessage!");
                break;
        }
    }

#if (UNITY_WEBGL || UNITY_IOS) && !(UNITY_EDITOR)
    const string DLL_NAME = "__Internal";
#else
    const string DLL_NAME = "UnityForCpp";
#endif

    //Expose C++ DLL methods from the TestPlugin.h file
    private class TestDLL
    {
        [DllImport(DLL_NAME)]
        public static extern void T_InitTest(int testReceiverId, int nOfGameObjects, float randomUpdatesInterval);

        [DllImport(DLL_NAME)]
        public static extern int T_UpdateTest(float deltaTime);

        [DllImport(DLL_NAME)]
        public static extern void T_TerminateTest();
    };
}
