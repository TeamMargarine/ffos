package com.spreadtrum.android.eng;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.util.Log;
import android.widget.Toast;

public class HsupaSetting extends PreferenceActivity implements Preference.OnPreferenceChangeListener{
	private static final String TAG = "HsupaSetting";
	private static final String KEY = "hsupa_setting";
	private int sockid = 0;
	private engfetch mEf;
	private CheckBoxPreference mCheckBoxPreference;
	private EventHandler mEventHandler;
	private Handler mHander = new Handler();
	private HandlerThread mThread;
	
	private static final int QUERY = 1;
	private static final int OPEN = 2;
	private static final int CLOSE = 3;
	
	@Override
	protected void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.layout.hsupa_setting);
		mCheckBoxPreference = (CheckBoxPreference) findPreference(KEY);
		mCheckBoxPreference.setOnPreferenceChangeListener(this);
		mThread = new HandlerThread(KEY);
		mThread.start();
		mEventHandler = new EventHandler(mThread.getLooper());
		initialpara();
	}

	private void initialpara(){
		mEf = new engfetch();
		sockid = mEf.engopen();
		Message m = mEventHandler.obtainMessage(QUERY);
		mEventHandler.sendMessage(m);
	}

	private class EventHandler extends Handler{
		public EventHandler(Looper looper) {
		    super(looper);
		}

		@Override
		public void handleMessage(Message msg) {
			ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
			DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);
			Log.e(TAG, "engopen sockid=" + sockid);
			String str;
			switch (msg.what) {
			case QUERY: {
				str = String.format("%d,%d",engconstents.ENG_AT_SPENGMD_QUERY,0);
				break;
			}
			case OPEN: {
				str = String.format("%d,%d",engconstents.ENG_AT_SPENGMD_OPEN,0);
				break;
			}
			case CLOSE: {
				str = String.format("%d,%d",engconstents.ENG_AT_SPENGMD_CLOSE,0);
				break;
			}
			default: {
				return;
			}
			}
			try {
				outputBufferStream.writeBytes(str);
			} catch (IOException e) {
				Log.e(TAG, "writebytes error");
				return;
			}
			mEf.engwrite(sockid, outputBuffer.toByteArray(),outputBuffer.toByteArray().length);

			int dataSize = 128;
			byte[] inputBytes = new byte[dataSize];
			int showlen = mEf.engread(sockid, inputBytes, dataSize);
			String str1 = new String(inputBytes, 0, showlen);
			Log.e(TAG, "str1=" + str1);
			if (str1.equals("1")) {
				setChecked(false);
			} else if (str1.equals("3")) {
				setChecked(true);
			} else if (str1.equals("OK")) {
				Toast.makeText(getApplicationContext(), "Set Success.",Toast.LENGTH_SHORT).show();
			} else if (str1.equals("error")) {
				Toast.makeText(getApplicationContext(), "Set Failed.",Toast.LENGTH_SHORT).show();
			} else {
//				Toast.makeText(getApplicationContext(), "Unknown",Toast.LENGTH_SHORT).show();
			}
		}
	}
	
	private void setChecked(final boolean checked){
		mHander.post(new Runnable() {
			public void run() {
				mCheckBoxPreference.setChecked(checked);
			}
		});
	}

	public boolean onPreferenceChange(Preference preference, Object newValue) {
		if(preference.getKey().equals(KEY)){
			boolean value = !mCheckBoxPreference.isChecked();
			mEventHandler.removeMessages(OPEN);
			mEventHandler.removeMessages(CLOSE);
			mEventHandler.removeMessages(QUERY);
			Message m = mEventHandler.obtainMessage(value ? OPEN:CLOSE);
			mEventHandler.sendMessage(m);
			return true;
		}
		return false;
	}

}
