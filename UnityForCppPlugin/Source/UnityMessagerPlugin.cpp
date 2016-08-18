//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

#include "Shared.h"
#include "UnityMessager.h"

using UnityForCpp::UnityMessager;

extern "C"
{
	int EXPORT_API UM_InitUnityMessagerAndGetControlQueueId(int maxNOfReceiverIds, int maxQueueArraysSizeInBytes)
	{
		return UnityMessager::InstanceAndProvideAwakeInfo(maxNOfReceiverIds, maxQueueArraysSizeInBytes);
	}

	void EXPORT_API UM_OnStartMessageDelivering()
	{
		UnityMessager::GetInstance().OnStartMessageDelivering();
	}

	void EXPORT_API UM_ReleasePossibleQueueArrays()
	{
		UnityMessager::GetInstance().ReleasePossibleQueueArrays();
	}

	void EXPORT_API UM_OnDestroy()
	{
		UnityMessager::GetInstance().DeleteInstance();
	}
}