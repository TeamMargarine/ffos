package com.spreadtrum.android.eng;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.text.method.NumberKeyListener;
import android.util.Log;
import android.view.Gravity;
import android.widget.Toast;

public class aocsettings extends PreferenceActivity 
implements Preference.OnPreferenceChangeListener{
	private static final String LOG_TAG = "aocsettings";
	private static final String KEY_AOCACT= "aoc_active";
	private static final String KEY_AOCSET = "aoc_setting";
	private static final String PIN2 = "2345";
	private String strInput;
	private int sockid = 0;
	private engfetch mEf;
	private EventHandler mHandler;
	private CheckBoxPreference mPreference01;
	private EditTextPreference mPreference02;
	
    private NumberKeyListener numberKeyListener = new NumberKeyListener() {

        private char[] numberChars = {
        		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
        };

        @Override
        public int getInputType() {
            return android.text.InputType.TYPE_CLASS_PHONE;
        }

        @Override
        protected char[] getAcceptedChars() {
            return numberChars;
        }
    };

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.layout.aocsettings);

		mPreference01 = (CheckBoxPreference) findPreference(KEY_AOCACT);
		mPreference01.setOnPreferenceChangeListener(this);
		mPreference02 = (EditTextPreference) findPreference(KEY_AOCSET);
		mPreference02.setOnPreferenceChangeListener(this);        
        mPreference02.getEditText().setKeyListener(numberKeyListener);
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

	private class EventHandler extends Handler{
		public EventHandler(Looper looper){
		    super(looper);
		}

		@Override
		public void handleMessage(Message msg) {
			ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
			DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);
			Log.d(LOG_TAG, "engopen sockid=" + sockid);

			try {
				switch (msg.what) {
				case engconstents.ENG_AT_CAOC:
				case engconstents.ENG_AT_CAOCD:
				case engconstents.ENG_AT_CAOCQ:
					outputBufferStream.writeBytes(String.format("%d,%d",msg.what, 0));
					break;
				case engconstents.ENG_AT_CAMM:
					outputBufferStream.writeBytes(String.format("%d,%d,%s,%s",msg.what, 2, strInput, PIN2));
					break;
				default:return;
				}
			} catch (IOException e) {
				Log.e(LOG_TAG, "writebytes error");
				return;
			}

			mEf.engwrite(sockid, outputBuffer.toByteArray(),outputBuffer.toByteArray().length);
			int dataSize = 128;
			byte[] inputBytes = new byte[dataSize];
			int showlen = mEf.engread(sockid, inputBytes, dataSize);
			String str1 = new String(inputBytes, 0, showlen);
			Log.d(LOG_TAG, "AT = " + msg.what + ";return = " + str1);
			if (str1.equals("OK")) {
				DisplayToast("Set Success.");
			} else if (str1.equals("ERROR")) {
				mPreference01.setChecked(!mPreference01.isChecked());//set to last state
				DisplayToast("Set Failed.");
			} else {
				mPreference01.setChecked(!mPreference01.isChecked());//set to last state
				DisplayToast("Unknown");
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
		if(KEY_AOCSET.equals(key)){
			mPreference02.setSummary(newValue.toString());
			strInput = newValue.toString();
			Message m = mHandler.obtainMessage(engconstents.ENG_AT_CAMM, 0, 0, 0);
			mHandler.sendMessage(m);
			return true;
		}else if(KEY_AOCACT.equals(key)){
			boolean checked = mPreference01.isChecked();
			if(checked){
				Message m = mHandler.obtainMessage(engconstents.ENG_AT_CAOC, 0, 0, 0);
				mHandler.sendMessage(m);
			}else{
				Message m = mHandler.obtainMessage(engconstents.ENG_AT_CAOCD, 0, 0, 0);
				mHandler.sendMessage(m);
			}
			return true;
		}
		else
			return false;
	}

}

