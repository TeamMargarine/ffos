package com.android.modemassert;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.app.ActivityManager;
import android.os.SystemProperties;
import java.lang.reflect.Method;

public class StartAssert extends BroadcastReceiver {
    private final String MTAG = "StartAssert";

    @Override
    public void onReceive(Context arg0, Intent arg1) {
        // TODO Auto-generated method stub
        String value;
        Log.d(MTAG, "receive boot complete\n");
        value = SystemProperties.get("persist.sys.sprd.modemreset", "default");
        if (value.equals("1")) {
            Log.d(MTAG, "SystemProperties value is equals 1...after modem assert, modem will be reset,"
                    + " this app do nothing, exit...\n");
            ActivityManager am = (ActivityManager) arg0
                    .getSystemService(Context.ACTIVITY_SERVICE);
            am.forceStopPackage("com.android.modemassert");
            return;
        }
        Intent mIntent;
        mIntent = new Intent(arg0, ModemAssert.class);
        arg0.startService(mIntent);
        Log.d(MTAG, "SystemProperties value is equals 0........, start ModemAssert class");
    }
}
