package com.spreadtrum.android.eng;

import android.app.Activity;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import android.view.View.OnClickListener; 
import android.os.Bundle;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;


import android.view.View;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;

public class SetIMEI extends Activity  implements OnClickListener {
    private static final String LOG_TAG = "engineeringmodel";
    private EditText     mIMEIEdit;
    private Button 		 mButtonR;
    private Button       mButtonW;
    private int  mInt01;

    private int          mSocketID = 0;
    private engfetch     mEf;
    private String       mATline = null;
    private String   	 mATResponse;
    private String 		 mIMEIInput = null;

    private ByteArrayOutputStream outputBuffer;
    private DataOutputStream outputBufferStream;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.setimei);

        mButtonR = (Button)findViewById(R.id.rbutton);	
        mButtonW = (Button)findViewById(R.id.wbutton);	
		mButtonR.setOnClickListener(this);
		mButtonW.setOnClickListener(this);

        mIMEIEdit = (EditText)findViewById(R.id.imei_edit);	
		mIMEIEdit.setText("");

        mEf = new engfetch();
        mSocketID = mEf.engopen();
    }


    @Override  
    public void onClick(View v) {  
		if (v == mButtonR) {
			mIMEIInput = ReadIMEI();
			if (mIMEIInput != null) {
				mIMEIEdit.setText(mIMEIInput);
			}
		}
		else if (v == mButtonW) {

		}
    }
	
    private String ReadIMEI() {
        outputBuffer = new ByteArrayOutputStream();
        outputBufferStream = new DataOutputStream(outputBuffer);

        Log.e(LOG_TAG, "Engmode socket open, id:" + mSocketID);
        mATline =String.format("%d,%d", engconstents.ENG_AT_REQUEST_IMEI, 0);

        try {
            outputBufferStream.writeBytes(mATline);
        } catch (IOException e) {
            Log.e(LOG_TAG, "writeBytes() error!");
            return null;
        }
        mEf.engwrite(mSocketID,outputBuffer.toByteArray(),outputBuffer.toByteArray().length);

        int dataSize = 128;
        byte[] inputBytes = new byte[dataSize];

        int showlen= mEf.engread(mSocketID, inputBytes, dataSize);
        mATResponse =  new String(inputBytes, 0, showlen);
        Log.d(LOG_TAG, "Read IMEI: " + mATResponse);
        if (mATResponse.equals("Error") == false) {
			return mATResponse;
        }
        else
            return null;
    }

    private int WriteIMEI(String IMEI) {
		return 0;
    }

	@Override
	protected void onDestroy() {
		mEf.engclose(mSocketID);
		super.onDestroy();
		Log.d(LOG_TAG, "setimei activity onDestroy.");
	}

}



