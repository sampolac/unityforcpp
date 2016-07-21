//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#ifndef SHARED_H
#define SHARED_H

#if _MSC_VER // this is defined when compiling with Visual Studio
#define EXPORT_API __declspec(dllexport) // Visual Studio needs annotating exported functions with this
#else
#define EXPORT_API // XCode does not need annotating exported functions, so define is empty
#endif

#ifndef WIN32 //have debug as default for when dropping cpp sources directly on Assets/Plugin folder for iOS and WebGL targets
#define _DEBUG
#endif

#ifndef NULL
#define NULL 0
#endif

typedef unsigned int uint;
typedef unsigned char uchar;

#ifdef WIN32
#define snprintf sprintf_s
#endif

#ifdef _DEBUG

//Wrap UnityAdapter functions into Shared namespace functions, avoiding creating a dependency with UnityAdapter.h
namespace Shared 
{
	void OutputDebugStr(const char* str);
	void OnAssertionFailed(const char* sourceFileName, int sourceLineNumber);
}

//Log str to Unity console using "Debug.Log"
#define DEBUG_LOG(str) Shared::OutputDebugStr(str)

//When exp is false, logs its source file name and line before triggering an usual C assertion 
#define ASSERT(exp) if (!(exp)) Shared::OnAssertionFailed(__FILE__, __LINE__)

#else //release version
#define DEBUG_LOG(str)
#define ASSERT(str)
#endif

#define DELETE(ptr) delete ptr; \
					ptr = NULL;

#endif