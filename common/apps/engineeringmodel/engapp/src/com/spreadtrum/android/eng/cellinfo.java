package com.spreadtrum.android.eng;

import android.app.Activity;
import android.util.Log;
import android.widget.TextView;
import android.os.Bundle;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import android.util.Log;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;

public class cellinfo extends Activity {
	private static final String LOG_TAG = "engnetinfo";
	private int sockid = 0;
	private engfetch mEf;
	private String str=null;
	private EventHandler mHandler;
	public TextView  tv;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

	mEf = new engfetch();
	sockid = mEf.engopen();

	Looper looper;
	looper = Looper.myLooper();
	mHandler = new EventHandler(looper);
	mHandler.removeMessages(0);

	Message m = mHandler.obtainMessage(engconstents.ENG_AT_CURRENT_BAND, 0, 0, 0);

	mHandler.sendMessage(m);

	tv = new TextView(this);
	tv.setText("");
	setContentView(tv);

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
		    case engconstents.ENG_AT_CURRENT_BAND:
			    ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
			    DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);

			    Log.e(LOG_TAG, "engopen sockid=" + sockid);
			    String strtemp = new String("?");	
			    str=String.format("%d,%s", msg.what,strtemp);
			    try {
			    	//outputBufferStream.writeBytes("pcclient");
			    	outputBufferStream.writeBytes(str);
			    } catch (IOException e) {
			        //Slog.e(TAG, "Unable to write package deletions!");
			    	Log.e(LOG_TAG, "writebytes error");
			       return;
			    }
			    mEf.engwrite(sockid,outputBuffer.toByteArray(),outputBuffer.toByteArray().length);

			    int dataSize = 128;
			    byte[] inputBytes = new byte[dataSize];
			    int showlen= mEf.engread(sockid,inputBytes,dataSize);
			    String str =new String(inputBytes,0,showlen); 
			    tv.setText(	str	);
			    break;
		 }
	}
}


}

