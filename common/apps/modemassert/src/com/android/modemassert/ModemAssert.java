package com.android.modemassert;

import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Binder;
import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.os.SystemClock;
import android.util.Log;

public class ModemAssert extends Service {
    private NotificationManager mNm;
    private final String MTAG = "ModemClient";
    private static final int MODEM_ASSERT_ID = 1;
    // private static final String MODEM_SOCKET_NAME = "modem";
    private static final String MODEM_SOCKET_NAME = "modemd";
    private LocalSocket mSocket;
    private LocalSocketAddress mSocketAddr;
    private CharSequence AssertInfo;

    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        return mBinder;
    }

    public void onCreate() {
        LocalSocket s = null;
        LocalSocketAddress l = null;

        // create local socket
        try {
            s = new LocalSocket();
            l = new LocalSocketAddress(MODEM_SOCKET_NAME,
                    LocalSocketAddress.Namespace.ABSTRACT);
            // add by lg for bug 20109 for 8810_2.3.5
            // l = new LocalSocketAddress(MODEM_SOCKET_NAME,
            // LocalSocketAddress.Namespace.ABSTRACT);
        } catch (Exception ex) {
            Log.w(MTAG, "create client socket Exception" + ex);
            if (s == null)
                Log.w(MTAG, "create client socket error\n");
        }
        mSocket = s;
        mSocketAddr = l;
        Thread thr = new Thread(null, mTask, "ModemAssert");
        thr.start();
    }

    public void onDestroy() {
        // print a string;
        Log.d(MTAG, "modem assert service destroyed\n");
    }

    private final IBinder mBinder = new Binder() {
        @Override
        protected boolean onTransact(int code, Parcel data, Parcel reply,
                int flag) throws RemoteException {
            return super.onTransact(code, data, reply, flag);
        }
    };

    final private int BUF_SIZE = 128;
    Runnable mTask = new Runnable() {
        public void run() {
            byte[] buf = new byte[BUF_SIZE];
            for (;;) {
                try {
                    mSocket.connect(mSocketAddr);
                    break;
                } catch (IOException ioe) {
                    Log.w(MTAG, "connect server error\n");
                    SystemClock.sleep(10000);
                    continue;
                }
            }

            synchronized (mBinder) {
                for (;;) {
                    int cnt = 0;
                    try {
                        // mBinder.wait(endtime - System.currentTimeMillis());
                        InputStream is = mSocket.getInputStream();
                        Log.d(MTAG, "read from socket: \n");
                        cnt = is.read(buf, 0, BUF_SIZE);
                        Log.d(MTAG, "read " + cnt + " bytes from socket: \n" );
                    } catch (IOException e) {
                        Log.w(MTAG, "read exception\n");
                    }
                    if (cnt > 0) {
                        String tempStr = "";
                        try {
                            tempStr = new String(buf, 0, cnt, "US-ASCII");
                        } catch (UnsupportedEncodingException e) {
                            // TODO Auto-generated catch block
                            Log.w(MTAG, "buf transfer char fail\n");
                        } catch (StringIndexOutOfBoundsException e) {
                            Log.w(MTAG, "StringIndexOutOfBoundsException\n");
                        }
                        AssertInfo = tempStr;
                        // Log.d(MTAG, ""+ tempStr);
                        showNotefication();
                        Log.v(MTAG, "exit modemassert app\n");
                        SystemClock.sleep(50000);
                    }
                    break;
                }
                ModemAssert.this.stopSelf();
            }
        }
    };

    private void showNotefication() {
        int icon = R.drawable.modem_assert;
        CharSequence thicktext = "modem assert";
        mNm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        long when = System.currentTimeMillis();
        Notification notification = new Notification(icon, thicktext, when);

        Context context = getApplicationContext();
        CharSequence contentTitle = "modem assert";
        CharSequence contentText = AssertInfo;
        Intent notificationIntent = new Intent(this, ModemAssert.class);
        PendingIntent contentIntent = PendingIntent.getService(context, 0,
                notificationIntent, 0);

        Log.e(MTAG, "Modem Assert!!!!\n");
        if (contentText != null)
            Log.e(MTAG, "" + contentText.toString());
        // notification.defaults |= Notification.DEFAULT_VIBRATE;
        long[] vibrate = { 0, 10000 };
        notification.vibrate = vibrate;

        notification.defaults |= Notification.DEFAULT_SOUND;
        // notification.sound = Uri.parse("file:///sdcard/assert.mp3");

        notification.setLatestEventInfo(context, contentTitle, contentText,
                contentIntent);
        mNm.notify(MODEM_ASSERT_ID, notification);
    }
}
