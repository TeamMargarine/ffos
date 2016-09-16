package com.spreadtrum.android.eng;

import android.app.Activity;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.os.Bundle;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import android.view.View;
import android.widget.Toast;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;

public class unlockfreq extends Activity {
	private static final String LOG_TAG = "engnetinfo";
	private int sockid = 0;
	private engfetch mEf;
	private String str=null;
	private EventHandler mHandler;
	public EditText  mET;
	private Button mButton;
	private Button mButton01;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.unlockfreq);

		initialpara();

		mET = (EditText)findViewById(R.id.editText1);
		mButton = (Button)findViewById(R.id.lock_button1);
		mButton01 = (Button)findViewById(R.id.clear_button1);
		clearEditText();
		mButton.setText("Unlock");
		mButton01.setText("Clear");

		mButton.setOnClickListener(new Button.OnClickListener(){
			public void onClick(View v) {
			// TODO Auto-generated method stub
			Log.d(LOG_TAG, "before engwrite");
			Message m = mHandler.obtainMessage(engconstents.ENG_AT_SETSPFRQ, 0, 0, 0);
			mHandler.sendMessage(m);
			Log.d(LOG_TAG, "after engwrite");
			}
		});

		mButton01.setOnClickListener(new Button.OnClickListener(){
			public void onClick(View v) {
			// TODO Auto-generated method stub
		    	clearEditText();
			}
		
		});
    	}

	private void clearEditText() {
		// TODO Auto-generated method stub
		mET.setText("0");
	}

	private void initialpara() {
		// TODO Auto-generated method stub
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
		public void handleMessage(Message msg){
			 switch(msg.what)
			 {
			    case engconstents.ENG_AT_SETSPFRQ:
					ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
					DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);

					Log.e(LOG_TAG, "engopen sockid=" + sockid);

					str=String.format("%d,%d,%d,%d,%d", msg.what,3,1,Integer.parseInt(mET.getText().toString()),10088);
					try {
					outputBufferStream.writeBytes(str);
					} catch (IOException e) {
					Log.e(LOG_TAG, "writebytes error");
					return;
					}
					mEf.engwrite(sockid,outputBuffer.toByteArray(),outputBuffer.toByteArray().length);

					int dataSize = 128;
					byte[] inputBytes = new byte[dataSize];
					int showlen= mEf.engread(sockid,inputBytes,dataSize);
					String str =new String(inputBytes,0,showlen); 

					if(str.equals("OK"))
					{
					Toast.makeText(getApplicationContext(), "Unlock Success.",Toast.LENGTH_SHORT).show(); 

					}
					else if(str.equals("ERROR"))
					{
					Toast.makeText(getApplicationContext(), "Unlock Failed.",Toast.LENGTH_SHORT).show(); 

					}
					else
					Toast.makeText(getApplicationContext(), "Unknown",Toast.LENGTH_SHORT).show(); 
				    break;
			 }
		}
	}


}

