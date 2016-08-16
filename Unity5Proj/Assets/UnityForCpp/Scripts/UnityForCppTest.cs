//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//This project is licensed under the terms of the MIT license

using UnityEngine;
using UnityEngine.Assertions;
using System.Collections;
using System;
using System.Runtime.InteropServices;
using UnityForCpp;

//This class works together with the TestPlugin.cpp file in order to test the features provided by UnityForCpp
public class UnityForCppTest : MonoBehaviour {

    private int[] m_sharedIntArray = null;
    private float[] m_sharedFloatArray = null;

	void Start () {
        Debug.Log("[UnityForCppTest] Start with testing the UnityForCpp plugin");

        //T_InitTest Performs first tests and request arrays we will access too from C#
        TestDLL.T_InitTest();

        UnityAdapter unityAdapter = UnityAdapter.Instance;
        m_sharedIntArray = unityAdapter.GetSharedArray<int>(TestDLL.T_GetSharedIntArrayId());
        m_sharedFloatArray = unityAdapter.GetSharedArray<float>(TestDLL.T_GetSharedFloatArrayId());

        if (m_sharedIntArray == null || m_sharedFloatArray == null)
        {
            Destroy(this.gameObject);
            throw new Exception("[UnityForCppTest] Could not retrieve one of the expected shared arrays");
        }

        Debug.Log("[UnityForCppTest] UnityForCpp testing has started successfully" +
                  "\n we will be testing the shared arrays once per second.");

        StartCoroutine(TestSharedArrays());
     }

    IEnumerator TestSharedArrays() {

        yield return new WaitForSeconds(1.0f);

        //the int array will be changed from C++ we will print it from C#
        TestDLL.T_ChangeSharedIntArray(); //expect the first 4 values to be incremented
        Debug.Log("[UnityForCppTest] First 4 values of the int array being printed from C#: "
                                + m_sharedIntArray[0].ToString()
                                + " " + m_sharedIntArray[1].ToString()
                                + " " + m_sharedIntArray[2].ToString()
                                + " " + m_sharedIntArray[3].ToString());

        //Not the float array is changed on C# and print from C++
        for (int i = 0; i < 4; ++i)
            m_sharedFloatArray[i] += 1;

        TestDLL.T_PrintSharedFloatArray();

        StartCoroutine(TestSharedArrays());
    }

    void OnDestroy()
    {
        Debug.Log("[UnityForCppTest] Ending the test.");
        TestDLL.T_TerminateTest();
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
        public static extern void T_InitTest();

        [DllImport(DLL_NAME)]
        public static extern int T_GetSharedIntArrayId();

        [DllImport(DLL_NAME)]
        public static extern int T_GetSharedFloatArrayId();

        [DllImport(DLL_NAME)]
        public static extern void T_ChangeSharedIntArray();

        [DllImport(DLL_NAME)]
        public static extern void T_PrintSharedFloatArray();

        [DllImport(DLL_NAME)]
        public static extern void T_TerminateTest();
    };
}
