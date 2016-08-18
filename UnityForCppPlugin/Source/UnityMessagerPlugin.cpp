//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

#include "Shared.h"
#include "UnityMessager.h"

using UnityForCpp::UnityMessager;

//This DLL exposed interface just wrap the respective UnityMessager methods used for the C# UnityMessager instance
//to communicate with the C++ UnityMessager instance. All communication from the C++ instance to the C# instance
//is made by sending "UnityMessager messages".
extern "C"
{
	//Check comments for UnityMessager::InstanceAndProvideAwakeInfo
	int EXPORT_API UM_InitUnityMessagerAndGetControlQueueId(int maxNOfReceiverIds, int maxQueueArraysSizeInBytes)
	{
		return UnityMessager::InstanceAndProvideAwakeInfo(maxNOfReceiverIds, maxQueueArraysSizeInBytes);
	}

	//Check comments for UnityMessager::OnStartMessageDelivering
	void EXPORT_API UM_OnStartMessageDelivering()
	{
		UnityMessager::GetInstance().OnStartMessageDelivering();
	}

	//Check comments for UnityMessager::ReleasePossibleQueueArrays
	void EXPORT_API UM_ReleasePossibleQueueArrays()
	{
		UnityMessager::GetInstance().ReleasePossibleQueueArrays();
	}

	//Check comments for UnityMessager::DeleteInstance
	void EXPORT_API UM_OnDestroy()
	{
		UnityMessager::GetInstance().DeleteInstance();
	}
}