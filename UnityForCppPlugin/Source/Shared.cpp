//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#include "Shared.h"
#include "UnityAdapter.h"

namespace Shared
{
char n_outputStrBuffer[OUTPUT_MESSAGE_MAX_STRING_SIZE] = {0};

void OutputDebugStr(const char* str)
{
	UnityForCpp::UnityAdapter::OutputDebugStr(UA_NORMAL_LOG, str);
}

void OutputWarningStr(const char* str)
{
	UnityForCpp::UnityAdapter::OutputDebugStr(UA_WARNING_LOG, str);
}

void OutputErrorStr(const char* str)
{
	UnityForCpp::UnityAdapter::OutputDebugStr(UA_ERROR_LOG, str);
}

}
