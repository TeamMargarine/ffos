package com.spreadtrum.android.eng;

import com.spreadtrum.android.eng.R;

import android.app.ProgressDialog;
import android.app.TabActivity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.widget.TabHost;
import android.widget.Toast;

public class LogSettingSlogUITabHostActivity extends TabActivity {
    /** Called when the activity is first created. */
    public static ProgressDialog mProgressDialog;
    private static Context sContext;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        TabHost tabHost = getTabHost();

        tabHost.addTab(tabHost
                .newTabSpec(SlogAction.GENERALKEY)
                .setIndicator(getText(R.string.tabtag_tabhost_tabgeneral))
                .setContent(
                        new Intent(this, LogSettingSlogUICommonControl.class)));

        tabHost.addTab(tabHost
                .newTabSpec(SlogAction.ANDROIDSPEC)
                .setIndicator(getText(R.string.tabtag_tabhost_tabandroid))
                .setContent(new Intent(this, LogSettingSlogUIAndroidPage.class)));

        tabHost.addTab(tabHost.newTabSpec(SlogAction.MODEMKEY)
                .setIndicator(getText(R.string.tabtag_tabhost_tabmodem))
                .setContent(new Intent(this, LogSettingSlogUIModemPage.class)));

        SlogAction.contextMainActivity = this;
        mProgressDialog = new ProgressDialog(
                LogSettingSlogUITabHostActivity.this);
        mProgressDialog
                .setMessage(getText(R.string.progressdialog_tabhost_prompt));
        mProgressDialog.setIndeterminate(true);
        mProgressDialog.setCancelable(true);
        mProgressDialog.setCanceledOnTouchOutside(false);

        mTabHostHandler.setContext(getApplicationContext());
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    public static SHandler mTabHostHandler = new SHandler();

    static class SHandler extends Handler {
        
        boolean isAndroidLog;
        int countAndroidLogBranchComplete;
        static Context mContext;

        public static void setContext(Context context) {
            mContext = context;
        }

        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {
            case SlogAction.ANDROIDKEY:
                isAndroidLog = true;
                countAndroidLogBranchComplete = 1;
                mProgressDialog.show();
                break;

            case SlogAction.MESSAGE_START_RUN:
                if (isAndroidLog) {
                    break;
                }
                mProgressDialog.show();
                break;

            case SlogAction.MESSAGE_END_RUN:
                if (isAndroidLog) {
                    if (++countAndroidLogBranchComplete == 4) {
                        isAndroidLog = false;
                    }
                }
                mProgressDialog.cancel();
                break;
            case SlogAction.MESSAGE_DUMP_START:
                mProgressDialog.setCancelable(false);
                mProgressDialog.show();
                break;
            case SlogAction.MESSAGE_DUMP_STOP:
                mProgressDialog.cancel();
                mProgressDialog.setCancelable(true);
                break;
            case SlogAction.MESSAGE_CLEAR_START:
                mProgressDialog.setCancelable(false);
                mProgressDialog.show();
                break;
            case SlogAction.MESSAGE_CLEAR_END:
                mProgressDialog.setCancelable(true);
                mProgressDialog.cancel();
                break;
            case SlogAction.MESSAGE_SNAP_SUCCESSED:
                if (mContext == null) {android.util.Log.e("SlogUIDebug","mContext==null");break;}
                Toast.makeText(mContext
                    , mContext.getText(R.string.toast_snap_success)
                    , Toast.LENGTH_SHORT )
                .show();
                break;
            case SlogAction.MESSAGE_SNAP_FAILED:
                if (mContext == null) break;
                Toast.makeText(mContext
                    , mContext.getText(R.string.toast_snap_failed)
                    , Toast.LENGTH_SHORT )
                .show();
                break;
            }

        }

    };
}
