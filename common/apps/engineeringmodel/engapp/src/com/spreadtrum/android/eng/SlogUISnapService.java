package com.spreadtrum.android.eng;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import com.spreadtrum.android.eng.R;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

/* SlogUI Added by Yuntao.xiao*/

public class SlogUISnapService extends Service {

	private static final Class[] mStartSnapSvcSignature = new Class[] {
			int.class, Notification.class };

	private static final Class[] mStopSnapSvcSignature = new Class[] { boolean.class };

	private Method mStartSnapSvc;
	private Method mStopSnapSvc;
	private Object[] mStartSnapSvcArgs = new Object[2];
	private Object[] mStopSnapArgs = new Object[1];
	private Notification notification;

	@Override
	public void onCreate() {
		// TODO Auto-generated method stub

		notification = new Notification(android.R.drawable.ic_btn_speak_now,
				getText(R.string.notification_snapsvc_statusbarprompt),
				System.currentTimeMillis());

		// The PendingIntent to launch our activity if the user selects this
		// notification
		PendingIntent contentIntent = PendingIntent.getActivity(
				SlogUISnapService.this, 0, new Intent(SlogUISnapService.this,
						SlogUISnapAction.class)
						.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
				PendingIntent.FLAG_UPDATE_CURRENT);

		// Set the info for the views that show in the notification panel.
		notification.setLatestEventInfo(SlogUISnapService.this,
				getText(R.string.notification_snapsvc_title),
				getText(R.string.notification_snapsvc_prompt), contentIntent);

		try {
			mStartSnapSvc = getClass().getMethod("startForeground",
					mStartSnapSvcSignature);
			mStopSnapSvc = getClass().getMethod("stopForeground",
					mStopSnapSvcSignature);
		} catch (NoSuchMethodException e) {
			// Running on an older platform.
			mStartSnapSvc = mStopSnapSvc = null;
		}

		Runnable runit = new Runnable() {

			public void run() {
				if (mStartSnapSvc != null) {
					mStartSnapSvcArgs[0] = Integer.valueOf(2);
					mStartSnapSvcArgs[1] = notification;
					try {
						mStartSnapSvc.invoke(SlogUISnapService.this,
								mStartSnapSvcArgs);
					} catch (InvocationTargetException e) {
						// Should not happen.
						Log.w("Slog", "Unable to invoke startForeground", e);
					} catch (IllegalAccessException e) {
						// Should not happen.
						Log.w("Slog", "Unable to invoke startForeground", e);
					}

				}
			}
		};

		Thread snapthread = new Thread(null, runit, "SnapThread");
		snapthread.start();
	}

	@Override
	public void onStart(Intent intent, int startId) {
		// TODO Auto-generated method stub
	}

	@Override
	public void onDestroy() {
		if (mStopSnapSvc != null) {
			mStopSnapArgs[0] = Boolean.TRUE;
			try {
				mStopSnapSvc.invoke(this, mStopSnapArgs);
			} catch (InvocationTargetException e) {
				// Should not happen.
				Log.w("ApiDemos", "Unable to invoke stopForeground", e);
			} catch (IllegalAccessException e) {
				// Should not happen.
				Log.w("ApiDemos", "Unable to invoke stopForeground", e);
			}
			return;
		}
		super.onDestroy();
	}

	@Override
	public IBinder onBind(Intent intent) {
		// TODO Auto-generated method stub
		return null;
	}

}
