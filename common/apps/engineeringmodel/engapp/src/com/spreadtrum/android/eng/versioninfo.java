package com.spreadtrum.android.eng;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.widget.TextView;

public class versioninfo extends Activity {
	private static final String TAG = "versioninfo";
	private int sockid = 0;
	private engfetch mEf;
	private String str = null;
	private TextView txtViewlabel01;
	private Handler mUiThread = new Handler();

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.version);
		txtViewlabel01 = (TextView) findViewById(R.id.version_id);
		txtViewlabel01.setText(R.string.imeiinfo);
		txtViewlabel01.setTextSize(20);

		mEf = new engfetch();
		sockid = mEf.engopen();

		new Thread(new Runnable() {
			@Override
			public void run() {
				requestEvent(engconstents.ENG_AT_CGMR);
			}
		}).start();
	}
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
	}
	
	private void requestEvent(int code){
		switch (code) {
		case engconstents.ENG_AT_CGMR:
			ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
			DataOutputStream outputBufferStream = new DataOutputStream(
					outputBuffer);

			str = String.format("%d,%d", code, 0);
			try {
				outputBufferStream.writeBytes(str);
			} catch (IOException e) {
				Log.e(TAG, "writebytes error");
				return;
			}
			mEf.engwrite(sockid, outputBuffer.toByteArray(),
					outputBuffer.toByteArray().length);

			int dataSize = 256;
			byte[] inputBytes = new byte[dataSize];
			int showlen = mEf.engread(sockid, inputBytes, dataSize);
			final String str = new String(inputBytes, 0, showlen);
			mUiThread.post(new Runnable() {
				public void run() {
					txtViewlabel01.setText(str);
				}
			});
			break;
		}
	}
}
