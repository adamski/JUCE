/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

package com.juce;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.http.SslError;
import android.net.Uri;
import android.os.Bundle;
import android.os.Looper;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelUuid;
import android.os.Environment;
import android.view.*;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.graphics.*;
import android.text.ClipboardManager;
import android.text.InputType;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.Pair;
import android.webkit.SslErrorHandler;
import android.webkit.WebChromeClient;
$$JuceAndroidWebViewImports$$         // If you get an error here, you need to re-save your project with the Projucer!
import android.webkit.WebView;
import android.webkit.WebViewClient;
import java.lang.Runnable;
import java.lang.ref.WeakReference;
import java.lang.reflect.*;
import java.util.*;
import android.media.AudioManager;
import android.Manifest;
import java.util.concurrent.CancellationException;
import java.util.concurrent.Future;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.Callable;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.atomic.*;
$$JuceAndroidMidiImports$$         // If you get an error here, you need to re-save your project with the Projucer!


//==============================================================================
public class JuceAppActivity   extends $$JuceAppActivityBaseClass$$
{
    private JuceBridge juceBridge;

    //==============================================================================
    public boolean isPermissionDeclaredInManifest (int permissionID)
    {
        String permissionToCheck = getAndroidPermissionName(permissionID);

        try
        {
            PackageInfo info = getPackageManager().getPackageInfo(getApplicationContext().getPackageName(), PackageManager.GET_PERMISSIONS);

            if (info.requestedPermissions != null)
                for (String permission : info.requestedPermissions)
                    if (permission.equals (permissionToCheck))
                        return true;
        }
        catch (PackageManager.NameNotFoundException e)
        {
            Log.d ("JUCE", "isPermissionDeclaredInManifest: PackageManager.NameNotFoundException = " + e.toString());
        }

        Log.d ("JUCE", "isPermissionDeclaredInManifest: could not find requested permission " + permissionToCheck);
        return false;
    }

    //==============================================================================
    // these have to match the values of enum PermissionID in C++ class RuntimePermissions:
    private static final int JUCE_PERMISSIONS_RECORD_AUDIO = 1;
    private static final int JUCE_PERMISSIONS_BLUETOOTH_MIDI = 2;
    private static final int JUCE_PERMISSIONS_READ_EXTERNAL_STORAGE = 3;
    private static final int JUCE_PERMISSIONS_WRITE_EXTERNAL_STORAGE = 4;

    private static String getAndroidPermissionName (int permissionID)
    {
        switch (permissionID)
        {
            case JUCE_PERMISSIONS_RECORD_AUDIO:           return Manifest.permission.RECORD_AUDIO;
            case JUCE_PERMISSIONS_BLUETOOTH_MIDI:         return Manifest.permission.ACCESS_COARSE_LOCATION;
                                                          // use string value as this is not defined in SDKs < 16
            case JUCE_PERMISSIONS_READ_EXTERNAL_STORAGE:  return "android.permission.READ_EXTERNAL_STORAGE";
            case JUCE_PERMISSIONS_WRITE_EXTERNAL_STORAGE: return Manifest.permission.WRITE_EXTERNAL_STORAGE;
        }

        // unknown permission ID!
        assert false;
        return new String();
    }

    public boolean isPermissionGranted (int permissionID)
    {
        return getApplicationContext().checkCallingOrSelfPermission (getAndroidPermissionName (permissionID)) == PackageManager.PERMISSION_GRANTED;
    }

    private Map<Integer, Long> permissionCallbackPtrMap;

    public void requestRuntimePermission (int permissionID, long ptrToCallback)
    {
        String permissionName = getAndroidPermissionName (permissionID);

        if (getApplicationContext().checkCallingOrSelfPermission (permissionName) != PackageManager.PERMISSION_GRANTED)
        {
            // remember callbackPtr, request permissions, and let onRequestPermissionResult call callback asynchronously
            permissionCallbackPtrMap.put (permissionID, ptrToCallback);
            requestPermissionsCompat (new String[]{permissionName}, permissionID);
        }
        else
        {
            // permissions were already granted before, we can call callback directly
            androidRuntimePermissionsCallback (true, ptrToCallback);
        }
    }

    private native void androidRuntimePermissionsCallback (boolean permissionWasGranted, long ptrToCallback);

    $$JuceAndroidRuntimePermissionsCode$$ // If you get an error here, you need to re-save your project with the Projucer!

    //==============================================================================
    public interface JuceMidiPort
    {
        boolean isInputPort();

        // start, stop does nothing on an output port
        void start();
        void stop();

        void close();

        // send will do nothing on an input port
        void sendMidi (byte[] msg, int offset, int count);
    }

    //==============================================================================
    $$JuceAndroidMidiCode$$ // If you get an error here, you need to re-save your project with the Projucer!

    //==============================================================================
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        juceBridge = JuceBridge.getInstance();
        juceBridge.setActivityContext(this);
        juceBridge.setScreenSaver(true);
        juceBridge.hideActionBar();
        setContentView(juceBridge.createViewForDefaultComponent(this));

        setVolumeControlStream(AudioManager.STREAM_MUSIC);

        permissionCallbackPtrMap = new HashMap<Integer, Long>();
    }

    @Override
    protected void onDestroy()
    {
        juceBridge.quitApp();
        super.onDestroy();

        juceBridge.clearDataCache();
    }

    @Override
    protected void onPause()
    {
        juceBridge.suspend();
        super.onPause();
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        juceBridge.resume();

    }

    @Override
    public void onRequestPermissionsResult (int permissionID, String permissions[], int[] grantResults)
    {
        juceBridge.onRequestPermissionsResult (permissionID, permissions, grantResults);
    }

    @Override
    public void onConfigurationChanged (Configuration cfg)
    {
        super.onConfigurationChanged (cfg);
        //setContentView (viewHolder); // TODO
    }

    // Need to override this as the default implementation always finishes the activity.
    @Override
    public void onBackPressed()
    {
        ComponentPeerView focusedView = juceBridge.getViewWithFocusOrDefaultView();

        if (focusedView == null)
            return;

        focusedView.backButtonPressed();
    }

}
