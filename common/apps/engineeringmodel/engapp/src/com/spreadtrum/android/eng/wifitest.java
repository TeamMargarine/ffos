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

public class wifitest extends Activity {
	private static final String LOG_TAG = "engnetinfo";
	private int sockid = 0;
	private engfetch mEf;
	private String str=null;
	public TextView  tv;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

	mEf = new engfetch();
	sockid = mEf.engopen();
	
	ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
	DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);


	str=String.format("%s%s", "CMD:","WIFI");
	try {
		outputBufferStream.writeBytes(str);
	} catch (IOException e) {
		Log.e(LOG_TAG, "writebytes error");
	   return;
	}
	mEf.engwrite(sockid,outputBuffer.toByteArray(),outputBuffer.toByteArray().length);

	mEf.engclose(sockid);
			
	tv = new TextView(this);
	tv.setText("Enter wifi test mode");
	setContentView(tv);

    }


    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

}

