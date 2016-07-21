//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#include "Shared.h"
#include "UnityAdapter.h"

namespace Shared
{
	void OutputDebugStr(const char* str)
	{
		UnityForCpp::UnityAdapter::OutputDebugStr(str);
	}

	void OnAssertionFailed(const char* sourceFileName, int sourceLineNumber)
	{
		UnityForCpp::UnityAdapter::OnAssertionFailed(__FILE__, __LINE__);
	}
}
