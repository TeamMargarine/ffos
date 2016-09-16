package com.spreadtrum.android.eng;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.widget.TextView;

public class adcCalibrateInfo extends Activity {
	private static final String LOG_TAG = "adcCalibrateInfo";
	private int sockid = 0;
	private engfetch mEf;
	private String str=null;
	private EventHandler mHandler;
	private TextView txtViewlabel01;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.adcinfo);
	txtViewlabel01= (TextView)findViewById(R.id.adc_id);
	txtViewlabel01.setTextSize(20);
	initial();

    }

    private void initial() {
		mEf = new engfetch();
		sockid = mEf.engopen();
		Looper looper;
		looper = Looper.myLooper();
		mHandler = new EventHandler(looper);
		mHandler.removeMessages(0);
		Message msg = mHandler.obtainMessage(engconstents.ENG_AT_SGMR, 0, 0, 0);
		mHandler.sendMessage(msg);
	}

	private class EventHandler extends Handler
    {
    	public EventHandler(Looper looper) {
    	    super(looper);
    	}

    	@Override
    	public void handleMessage(Message msg) {
    		 switch(msg.what)
    		 {
                    case engconstents.ENG_AT_SGMR:
			ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
			DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);

			Log.e(LOG_TAG, "engopen sockid=" + sockid);

			str=String.format("%d,%d,%d,%d,%d", msg.what,3,0,0,3);
			try {
			    outputBufferStream.writeBytes(str);
			} catch (IOException e) {
			    Log.e(LOG_TAG, "writebytes error");
			return;
			}
			mEf.engwrite(sockid,outputBuffer.toByteArray(),outputBuffer.toByteArray().length);

			int dataSize = 512;
			byte[] inputBytes = new byte[dataSize];
			int showlen= mEf.engread(sockid,inputBytes,dataSize);
			String str =  new String(inputBytes, 0, showlen);
			txtViewlabel01.setText(str);
			break;
    		 }
    	}
    }    
}
