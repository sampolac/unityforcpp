//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#if _MSC_VER // this is defined when compiling with Visual Studio
#define EXPORT_API __declspec(dllexport) // Visual Studio needs annotating exported functions with this
#else
#define EXPORT_API // XCode does not need annotating exported functions, so define is empty
#endif

#ifndef WIN32 //have debug as default when dropping cpp sources directly on Assets/Plugin folder for iOS and WebGL targets
#define _DEBUG
#endif

#ifndef NULL
#define NULL 0
#endif

typedef unsigned int uint;

#ifdef WIN32
#define snprintf sprintf_s
#endif

