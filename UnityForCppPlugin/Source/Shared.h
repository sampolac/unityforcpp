//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

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

#include <stdint.h>

typedef int8_t int8; //char
typedef uint8_t uint8; //unsigned char
typedef int16_t int16; //short
typedef uint16_t uint16; //unsigned short
typedef int32_t int32; //int
typedef uint32_t uint32; //unsigned int
typedef long long int64; //long long
typedef unsigned long long uint64; //unsigned long long


#include "stdio.h"

#ifdef WIN32
#define snprintf sprintf_s
#endif

#ifdef _DEBUG

#ifndef WIN32
#include <assert.h>
#else
#include <crtdbg.h>
#define assert _ASSERT
#endif

//Wrap UnityAdapter functions into Shared namespace functions, avoiding creating a dependency with UnityAdapter.h
#define OUTPUT_MESSAGE_MAX_STRING_SIZE 1024

namespace Shared
{
extern char n_outputStrBuffer[OUTPUT_MESSAGE_MAX_STRING_SIZE];

void OutputDebugStr(const char* str);
void OutputWarningStr(const char* str);
void OutputErrorStr(const char* str);
}

#define LOG_WITH_PARAM_HELPER(LogMacro, messageFormatStr, ...) {\
	snprintf(Shared::n_outputStrBuffer, OUTPUT_MESSAGE_MAX_STRING_SIZE - 1, messageFormatStr, __VA_ARGS__);\
	LogMacro(Shared::n_outputStrBuffer);\
	}

//Log str to Unity console using "Debug.Log"
#define DEBUG_LOG(str) Shared::OutputDebugStr(str)
#define DEBUG_LOGF(formatStr, ...) LOG_WITH_PARAM_HELPER(DEBUG_LOG, formatStr, __VA_ARGS__)

//Log str to Unity console using "Debug.LogWarning"
#define WARNING_LOG(str) Shared::OutputWarningStr(str)
#define WARNING_LOGF(formatStr, ...) LOG_WITH_PARAM_HELPER(WARNING_LOG, formatStr, __VA_ARGS__)

//Log str to Unity console using "Debug.LogError"
#define ERROR_LOG(str) Shared::OutputErrorStr(str)
#define ERROR_LOGF(formatStr, ...) LOG_WITH_PARAM_HELPER(ERROR_LOG, formatStr, __VA_ARGS__)

//When exp is false, logs its source file name and line before triggering an usual C assertion 
#define ASSERT(exp) if (!(exp)) { ERROR_LOGF("ASSERTION FAILED: file %s, line %d", __FILE__, __LINE__); assert(false); }

#else //release version
#define DEBUG_LOG(str)
#define DEBUG_LOGF(formatStr, ...)

#define WARNING_LOG(str)
#define WARNING_LOGF(formatStr, ...)

#define ERROR_LOG(str) 
#define ERROR_LOGF(formatStr, ...) 

#define ASSERT(str)
#endif

#define DELETE(ptr) { delete ptr; \
					  ptr = NULL; }

#endif