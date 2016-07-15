<h1>Unity For C++
</h1>


<p>The goal for this project is to provide a reasonable, tough not fully featured, C++ game development framework based on Unity. Actually, it is a much simpler project than it may sound. The planned outcome is a kind of View-Controller framework, in such way only the Controller side is done on C++ and the View side still being done as usual in Unity. </p>


<p>So, we are not bringing the same Unity API features we have at C# to C++. Instead, we take advantage of Unity for providing essential multi-platform solutions to the C++ code, such as for file system accessing, logging, internet access, etc.. Rendering/view related information and features are not expected to be provided. Instead, we focus on building an efficient communication pipe from C++ to C# for streaming game logic messages and events in such way we can provide a proper view to this game logic using C#.
</p>


<h3>Unity For C++ Is NOT Intended For:
</h3>


<p><strong>- Small projects and prototypes:</strong> Unity itself is great and more efficient for getting these done. Unity for C++ focus goes towards medium to large projects, where experienced C++ coders may feel more comfortable by having full control of things, at least when implementing the gameplay logic, also being able to build a code base that is "engine-independent", while still taking advantage of so many features Unity has to offer. </p>



<p><strong>- Being used together with the Unity Low-Level Plugin Interface:</strong> other C++ developers could may want to use it this way, and I believe that could work fine. However, given some particular choices of my own indie project, I want my C++ code to be independent from Unity itself. Despite of the fact, I am using names such as "UnityAdapter" on my code, this plugin DLL could be connected to any other C# based engine or even with other C++ code directly. Also, if this project were using the low-level renderer interface we would not be really putting the view on the Unity/C# side, which is main reason for my choice in using Unity with my C++ code base. This way I can co-work fine with a huge number of Unity developers, including less experienced ones and I can get close to the traditional Unity workflow and also use lots of asset store stuff.</p>


<p><strong>- Learning C++ coding and/or Unity development:</strong> Making the proper separation from View and Controller for your specific game logic is not trivial for those learning how to code and the usage of this alternative architecture differs a lot from the usual way things are done in Unity.
</p>


<h3>Status and What Comes Next </h3>


<p><strong>What we have now:</strong> The first version of the UnityAdapter interface, providing to the C++ code synchronous access to Unity logging, to the Unity file system for reading and writing and to shared arrays of the main C#/C++ built-in types.
</p>

<p><strong>What I am currently working on:</strong> The "UnityMessager" class which uses the shared arrays got from the UnityAdapter to build an streaming pipe of asynchronous messages from C++ to C#. Having that, view related events on the C++ side will be handled by traditional Unity objects at the C# side providing the game view. 
</p>


<h3>Tested platforms</h3>



<p>The code should work on all possible platforms that accept C++ plugins. I have tested the current code with Windows, iOS and WebGL builds. For testing on WebGL/iOS just drop all the .h and .cpp files at the Assets/UnityForCpp/Plugins folder and select them to be part of the iOS and WebGL builds. 
</p>


<p>This suggestion of just dropping the source files at the Plugins folder is for my own convenience on maintaining this repository. It actually brings a lot of unacceptable limitations for bigger C++ projects. UnityForCpp is actually a subproject of my main C++ project, for it, I using emscripten on generating a ".bc" library for WebGL builds and XCode on generating an ".a" library for iOS builds.
</p>


<h3>Demo
</h3>


<p>This project itself is a demo. It has a testing specific source file at the C++ side, that works together with another test specific source at the C# code, testing each one of the features currently implemented and printing meaningful data to the console. 
</p>


<p>You can see the console log for WebGL builds on your browser dev console. For checking it working on iOS devices, without being debugging from XCode, you may want to include "Log Viewer" (Asset Store) on your project.
</p>


<h3>Documentation
</h3>


<p>Currently there is no official documentation for this project, but I have made a considerable effort in commenting the interfaces properly in such way is easy to use them, in special referring to how they are used on the test files. 
</p>

<h3>Collaborate
</h3>


<p>Collaborators are welcome. If you find this project was useful for you and have added some new interesting feature to it, please write me, preferably on my personal e-mail: samuelpolacchini@gmail.com .</p>










