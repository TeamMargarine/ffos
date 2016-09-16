package com.spreadtrum.android.eng;

import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;
import android.webkit.WebView;
import android.util.Log;

public class uaagent extends Activity {
    private static final String LOG_TAG = "uaagent";
    private TextView tv = null;
    private WebView mWebView = null;
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.uaagent);
        tv = (TextView)findViewById(R.id.uaagent);
        mWebView = new WebView(this);
        String mUserAgent =mWebView.getSettings().getUserAgentString();
        Log.d(LOG_TAG, "UserAgent is <"+mUserAgent+">");
        tv.setText(mUserAgent);
    }
}

