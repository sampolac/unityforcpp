#Unity For C++

## Contents

- [What](#what)
- [Features and Usage](#features-and-usage)
  - [Unity Messager - Sending messages from C++ to C# almost like making direct calls to C# interfaces](#unity-messager)
  - [Unity Array - Shared Arrays between C# and C++](#unity-arrays)
  - [File reading/writing through Unity](#file-features)
  - [Logging/Debugging utilities](#logging-debugging)
- [Why Unity For C++?](#why-unity-for)
- [Installing and Setup](#installing-and-setup)
- [Collaboration and Bug Reports](#collaboration-and-bug-reports)

## What:
	
- Unity for C++ provides a framework for developing Unity games keeping a considerable part of our code as a very portable and independent C++ code. This C++ code may rely on unity for providing multi-platform features (such as input, file system access, network/internet access). The "output" of the game (graphics and sounds) can be implemented as usual in Unity 3D with C#, taking advantage of this powerful engine and its numerous available assets.
- Unity for C++ IS NOT based on the Unity Low-Level Plugin Interface or on C++/CLI. It uses shared arrays for sending messages (similar to an interprocess communication protocol) and, sometimes, reverse Pinvoke for some critical stuff, for which immediate return from the C# code is needed. 
- Unity for C++ is STRONGLY TYPED on both sides (generics on C# and templates on C++), not making use of "unsafe" C# code and also not relying on crazy casts at the C++ side.  
- Unity for C++ was designed to work on any platform supported by Unity, tough its development is being made on Windows and, before any commit to this repo, it is also being tested on iOS and WebGL platforms through Unity cloud builds. 

## Features and Usage:

###<a name="unity-messager">Unity Messager - Sending messages from C++ to C# almost like making direct calls to C# interfaces</a>

**Basic Usage:** The easiest and most straightforward way of sending a message from the C++ looks like:

```
//on MyComponentType.cs file, which is a component of a game object named "MyObject" in the current scene.
void MyMethod(int param1, float param2, Vec2 param3, string param4, int[] param5) { /*C# implementation*/ }

//on the .h file:
UM_DECLARE_COMPONENT(MyComponentType); //this is all we need before being able to use a component type on C++

//on the .cpp implementation:
UNITY_MESSAGER.SendMessage<MyComponentType>("MyObject", "MyMethod", 
										5, 2.3f, Vec2(0.2f, 2.1f), "string parameter", 
										UM_ARRAY_PARAM(intCArray, length));
```

It works like an improved "UnitySendMessage", but with support to *several parameter types and in any number or order*. Also, unlike "UnitySendMessage", this one works on C++ plugins for *any platform*. The game object is find via "GameObject.Find" and the method is called via reflection with the provided method name and parameters. For understanding the supported types check the *Type Support* item on the next section ("Unity Array").

**Improved Usage:** It is possible to avoid using "GameObject.Find" and reflection, replacing them by fast indexed operations. For the example above, we would have (on C++):

```
UNITY_MESSAGER.SendMessage<MyComponentType>(receiverId, MESSAGE_ID, 5, 2.3f, Vec2(0.2f, 2.1f), "string parameter", UM_ARRAY_PARAM(intCArray, length));
```

"MESSAGE_ID" is an integer value on our control to identify different message types. The "receiverId" is an integer value that works like an "address" for our C# object instance. We can request receiver ids to UnityMessager from both sides (C++ or C#). Having a receiver id we can bound C# objects to it by calling UnityMessager.Instance.SetReceiverObject(receiverId, receiverInstance), or its equivalent for game objects, which can be used for messages sent to game object components.

Receiver C# classes (MyComponentType on the example) must implement the UnityMessager.IMessageReceiver interface, which specifies only one method to be implemented, that would looks like this way for the C++ message exemplified above:

```
	public void ReceiveMessage(ref UnityMessager.Message msg)
        {
            switch (msg.MessageId)
            {
			case MESSAGE_ID:
				MyMethod(msg.ReadNextParamAndAdvance<int>(),
						   msg.ReadNextParamAndAdvance<float>(),
						   msg.ReadNextParamAndAdvance<Vec2>(),
						   msg.ReadNextParamAsStringAndAdvance(),
						   msg.ReadNextParamAsArrayAndAdvance<int>());
			//...
			}
		}
```

This example suggests messages should be routed to regular C# methods within the same instance, however we are free to use messages as we prefer since the UnityMessager.Message received as parameter also offers methods for querying the message in the details about the type of each parameter received. 

**Recommended usage (demo project):** Close to the top of UnityForCppTest.cs and also Test.h the comment "START OF THE EXPOSED INTERFACE" introduces the C# methods *exposed* and their C++ signature. This is how I recommend using it because it is like if we could really expose C# methods to the C++ code, making the process of supporting this exposure a mostly mechanic process, which may be automated by a Python script in the future. 

The main gain of this practice for now is to assure, at compilation time, that we are not sending wrong messages from the C++ side in terms of the parameter types, or in terms of the number or order of the parameters. For the sample C++ message above the wrapped message into a C++ method signature would look this way:

```
void MyMethod(int intParam, float floatParam, Vec2 vec2Param, const char* strParam, const vector<int>& arrayParam) 
{
	UNITY_MESSAGER.SendMessage(receiverId, MESSAGE_ID, intParam, floatParam, vec2Param, strParam, UM_ARRAY_PARAM(arrayParam.data(), arrayParam.size()));
}
```


###<a name="unity-arrays">Unity Array - Shared Arrays between C# and C++</a>

**Basic Usage:** Creating and using the UnityArray C++ class is pretty straightforward:

```
UnityArray<float> myArray;
myArray.Alloc(length); 
myArray[0] = 1.0f;
```  

In order to access this same array from C# we should pass its id (with myArray.GetId()) to the C# code (using a message for example) and so we can retrieve it as a C# native array this way:

```
float[] myArray = UnityAdapter.Instance.GetSharedArray<float>(arrayId);
```

The act of instancing or destroying a shared array always come from the C++ logic through the UnityArray class. The destructor for it already takes care of properly releasing the shared array. However, if an UnityArray instance is not being deleted when OnDestroy is called for an Unity game, a call to the Release method (myArray.Release()) MUST be made on OnDestroy, so no "dirty" states remain between different game executions within the Unity Editor.
 
**Type Support:** All the blittable types are supported by default (https://msdn.microsoft.com/en-us/library/75dwhxf7.aspx). By using the macro UA_SUPPORTED_TYPE, structs containing these types can also be supported, as long as they do not contain C arrays, which cannot be taken as native C# arrays.

**Demo Project:** The demo project shows an UnityArray of custom type (Vec2) being used to provide the positions (from C++ to C#) of several game objects at each frame update.   

###<a name="file-features">File reading/writing through Unity</a>

**Supported Operations:** Reading, writing and deleting files are the supported operations. For all the operations the "UnityEngine.Application.persistentDataPath" is considered to be root path for the passed path from the C++ code. Additionally, the read operation automatically looks for files under Asset/Resources when that first search mentioned fails. 
 
**Demo Project:** Reading and Writing a file content, as well as deleting a file, are pretty straightforward operations, well exemplified by the method "TestFileRelatedFeatures" in the Test.cpp file. 

###<a name="logging-debugging">Logging/Debugging utilities</a>
 
**Supported Operations:** The macros DEBUG_LOGF, WARNING_LOGF and ERROR_LOGF may be used to log formatted strings from the C++ code (similarly to "printf"). These operations however are quite expensive in terms of performance and should be used carefully. A convenient ASSERT macro is also provided, logging the C++ file name and line to the Unity console (and log file) before triggering a regular C assertion on fail.     

**Demo Project:** The demo project has several usage examples for these macros. 

## Why Unity For C++?

Unity For C++ strives for putting together the  "best of two worlds" when it comes to long term game development, in other words, when it comes to game/engine projects with years of development ahead, in special when the resulting code base is planned for decades of reuse (my case). This is why: 

**C++ code can live forever:** Games/Engines on pure C++ code, without platforms dependencies, live forever with close to zero maintenance cost. An example is Box2D, it is a great 2D engine today and will still be in years from now with its same current code base. Engines with rendering features, like Unity, are constantly changing, for improvements and for supporting an increasing number of changes on numerous platforms. Also, these engines may just disappear or stay a lot behind from the powerful new engines the future will bring. Unity is an industry standard of "today", in the same way Flash was an industry standard some years ago...  

**C++ code can be free:** While Unity is great for what we are picturing for our games now, this may not be always the case. When using Unity For C++, out C++ code gets almost no dependencies from Unity itself, so it is easy to bring it to other engines when they are open source on its C++ code or support C#/C++ interop. Godot and Atomic Engine are examples of open source engines that are getting better each day and both are currently on the process of supporting C# scripting.     

**Unity is awesome:** It is hard to deny the benefits Unity brings to low-budget casual game projects, not only in terms of features and multi-platform support, but also in terms of availability of assets to be integrated on our projects and availability of capable programmers to join our teams. By keeping the "view" side of the game on the usual C# code, Unity For C++ brings the best of Unity to C++ projects, in such way the art pipeline remains the same and optionally the level design process too. Additionally, the C++ code can totally rely on Unity for everything that is platform specific or that is already handled by Unity itself or by assets on the Asset Store, such as supporting multiple kind of input devices, accessing the file system or the internet.

**HOWEVER**: It is good to highlight that implementing and debugging C++ code directly with Unity is *hard*, mainly due to the need of constantly restarting the editor for loading DLL changes. All my code base was developed with Cocos2d-x with very basic rendering features and this still the way I go with when implementing new features to the C++ "game logic", being Unity used for providing high quality rendering, sounds and multi-platform features to this "game logic".   

## Installing and Setup:

1. **Prerequisites:** This project was developed under Windows 8.1, Unity 5.3.4f1 and Visual Studio Professional 2013 Update 5 with Microsoft Visual Studio Tools For Unity enabled. Things should work fine within this software versions and above.

2. **Demo Project:** The content of this repository includes a test project focused on properly testing and providing usage examples of all the relevant features currently implemented. Get it running is the first step before using UnityForCpp on your own project. 

3. **Importing:** Drag and drop the "UnityForCpp" folder under Assets to your Assets folder. You may also export it as a package from the demo project and import to your project. Any file containing "Test" on its name is not essential. Once imported, drag and drop the UnityForCpp prefab to your scene and update the DLL_NAME value to your dll name on UnityAdapter and UnityMessager scripts. At the plugin side (C++) just bring all the source files, except those with "Test" on their names, to your VS project.

4. **Script Order:** Before the execution of any script that makes usage of your DLL/Plugin, the UnityAdapter.cs script must be executed, followed by UnityMessager.cs. Also your project should have a main and unique script to communicate the game execution start (Awake or Start) and termination (OnDestroy), in such way your C++ code has reference points to release all the allocated shared arrays and reset the game to its initial state considering the DLL state is not reset when the editor stops and replays the game within a same editor execution. 
 
5. **Supported Platforms**: The code should work on all possible platforms that accept C++ plugins. I have tested the current code on Windows, iOS and WebGL builds. For quickly testing on WebGL/iOS just drop all the C++ source and header files at Assets/UnityForCpp/Plugins and select them to be part of the iOS and WebGL builds. This suggestion of dropping C++ sources directly is for my own convenience on maintaining this repository. It actually brings a lot of unacceptable limitations on bigger C++ projects, for those we should use emscripten for generating a ".bc" library for WebGL builds and XCode for generating an ".a" library for iOS builds.

##Collaboration and Bug Reports

Collaboration and bug reports are more than welcome. Just send me an e-mail at: samuelpolacchini@gmail.com .





 




