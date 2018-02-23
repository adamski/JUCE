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
import android.os.Bundle;	
import android.content.Intent;
import android.content.res.Configuration;
import android.media.AudioManager;
import com.juce.JuceBridge;


//==============================================================================
public class JuceAppActivity   extends $$JuceAppActivityBaseClass$$
{
    private JuceBridge juceBridge;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        juceBridge = JuceBridge.getInstance();
        juceBridge.setActivityContext(this);
        juceBridge.setScreenSaver(true);
        juceBridge.hideActionBar();
        setContentView(juceBridge.getViewHolder());

        setVolumeControlStream(AudioManager.STREAM_MUSIC);

        //permissionCallbackPtrMap = new HashMap<Integer, Long>();
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
        juceBridge.handleSuspend();
        super.onPause();
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        juceBridge.handleResume();

    }

    @Override
    public void onRequestPermissionsResult (int permissionID, String permissions[], int[] grantResults)
    {
        // TODO: Fix
        //juceBridge.onRequestPermissionsResult (permissionID, permissions, grantResults);
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
        juceBridge.handleBackPressed();
    }

    @Override
    protected void onActivityResult (int requestCode, int resultCode, Intent data)
    {
        juceBridge.appActivityResult (requestCode, resultCode, data);
    }

    @Override
    protected void onNewIntent (Intent intent)
    {
        super.onNewIntent(intent);
        setIntent(intent);

        juceBridge.appNewIntent (intent);
    }

}
