package com.spreadtrum.android.eng;

import android.util.Log;
import android.widget.Toast;
import android.os.Bundle;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.view.Gravity;

public class logswitch extends PreferenceActivity 
implements Preference.OnPreferenceChangeListener{
	private static final String LOG_TAG = "logswitch";
	private static final String KEY_INTEG = "integrity_set";
	private static final String KEY_FBAND = "fband_set";
	public static final String KEY_CAPLOG = "cap_log";
	private static final int setIntegrity = 1;
	private static final int setFBAND = 11;
	private int openlog = 0;
	private int sockid = 0;
	private engfetch mEf;
	private EventHandler mHandler;
	private CheckBoxPreference mPreference03,mPreference04,mPreference05;

	

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.layout.logswitch);

		mPreference03 = (CheckBoxPreference) findPreference(KEY_INTEG);
		mPreference03.setOnPreferenceChangeListener(this);    
		mPreference04 = (CheckBoxPreference) findPreference(KEY_FBAND);
		mPreference04.setOnPreferenceChangeListener(this);    
		mPreference05 = (CheckBoxPreference) findPreference(KEY_CAPLOG);
		mPreference05.setOnPreferenceChangeListener(this);
		initialpara();
	}

	private void initialpara(){
		mEf = new engfetch();
		sockid = mEf.engopen();
		Looper looper;
		looper = Looper.myLooper();
		mHandler = new EventHandler(looper);
		mHandler.removeMessages(0);
	}

	@Override
	protected void onStop(){
	super.onStop();
	}

	@Override
	protected void onDestroy() {
	super.onDestroy();
	}

	private class EventHandler extends Handler{
		public EventHandler(Looper looper){
		    super(looper);
		}

		@Override
		public void handleMessage(Message msg) {
			switch(msg.what){
				case engconstents.ENG_AT_SETARMLOG:
				case engconstents.ENG_AT_SETSPTEST:
				case engconstents.ENG_AT_SETCAPLOG:

				ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
				DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);

				Log.e(LOG_TAG, "engopen sockid=" + sockid);
				try {
					if(engconstents.ENG_AT_SETARMLOG == msg.what)
				    outputBufferStream.writeBytes(String.format("%d,%d,%d", msg.what,1,openlog));
					else if(engconstents.ENG_AT_SETCAPLOG == msg.what)
					outputBufferStream.writeBytes(String.format("%d,%d,%d", msg.what,1,openlog));
					else
					outputBufferStream.writeBytes(String.format("%d,%d,%d,%d", msg.what,2,msg.arg1,msg.arg2));	
				} catch (IOException e) {
				Log.e(LOG_TAG, "writebytes error");
				return;
				}
				mEf.engwrite(sockid,outputBuffer.toByteArray(),outputBuffer.toByteArray().length);
				Log.d(LOG_TAG, "after engwrite");
				int dataSize = 512;
				byte[] inputBytes = new byte[dataSize];
				int showlen= mEf.engread(sockid,inputBytes,dataSize);
				String str1 =new String(inputBytes,0,showlen); 

				if(str1.equals("OK")){
				DisplayToast("Set Success.");
				}
				else if(str1.equals("ERROR")){
				DisplayToast("Set Failed.");
				}
				else
				DisplayToast("Unknown");
			    break;

			 }
		}
	}

	private void DisplayToast(String str) {
		Toast mToast = Toast.makeText(this, str, Toast.LENGTH_SHORT);
		mToast.setGravity(Gravity.TOP, 0, 100);
		mToast.show();
	}

	public boolean onPreferenceChange(Preference preference, Object newValue) {
		// TODO Auto-generated method stub
		final String key = preference.getKey();
		Log.d(LOG_TAG, "onPreferenceChange newValue.toString() = "+newValue.toString());
		
		if(newValue.toString().equals("true"))
			openlog = 1;
			else if(newValue.toString().equals("false"))
			openlog = 0;

		if(KEY_INTEG.equals(key)){
			Message m = mHandler.obtainMessage(engconstents.ENG_AT_SETSPTEST, setIntegrity, openlog, 0);
			mHandler.sendMessage(m);
			return true;
		}else if(KEY_FBAND.equals(key)){
			Message m = mHandler.obtainMessage(engconstents.ENG_AT_SETSPTEST, setFBAND, openlog, 0);
			mHandler.sendMessage(m);
			return true;
		}else if(KEY_CAPLOG.equals(key)){
			Message m = mHandler.obtainMessage(engconstents.ENG_AT_SETCAPLOG, 0, 0, 0);
			mHandler.sendMessage(m);
			return true;
		}
		else
			return false;
	}

}

