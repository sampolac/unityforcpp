//Copyright (c) 2016, Samuel Pollachini (Samuel Polacchini)
//The UnityForCpp project is licensed under the terms of the MIT license

using UnityEngine;
using UnityEngine.Assertions;
using System.Collections;
using System;
using System.Runtime.InteropServices;
using UnityForCpp;

//Simple component for testing message receiving from the C++ code 
public class ReceiverComponentTest : MonoBehaviour, UnityMessager.IMessageReceiver
{
    //property we want to update from C++ for testing purposes, Vector3 is not supported, but Vec2 supports an implicit conversion to it.
    public Vector3 Scale
    {
        get { return _scale; }
        set
        {
            _scale = value;
            _transform.localScale = new Vector3(_scale.x * _originalScale.x,
                                                               _scale.y * _originalScale.y,
                                                               _originalScale.z);
        }
    }

    //Just for a quick test, by calling it from cpp with SendMessage<ReceiverComponentTest>(receiverId, "TestReflectionBasedMessage", ...)
    public void TestReflectionBasedMessage(int intValue, float floatValue, byte[] array, string strValue)
    {
        Debug.Log("[ReceiverComponentTest] TestReflectionBasedMessage called!");
    }

    private Vector3 _scale;

    //we keep a backup of this, so when scaling we preserve this original reference value
    private Vector3 _originalScale;
    private Transform _transform;

    public void ReceiveMessage(ref UnityMessager.Message msg)
    {
        switch(msg.MessageId)
        {
            case 0:
            {
                Scale = msg.ReadNextParamAndAdvance<UnityForCppTest.Vec2>(); //scale

                break;
            }
        }
    }

    private void Awake()
    {
        _scale = new Vector3(1.0f, 1.0f, 1.0f);

        _transform = GetComponent<Transform>();
        _originalScale = transform.localScale;
    }
}
