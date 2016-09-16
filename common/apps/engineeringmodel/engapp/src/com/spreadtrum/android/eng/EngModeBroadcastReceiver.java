package com.spreadtrum.android.eng;

import static android.provider.Telephony.Intents.SECRET_CODE_ACTION;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;

public class EngModeBroadcastReceiver extends BroadcastReceiver {
	private static final String TAG = "EngModeBroadcastReceiver";

	@Override
	public void onReceive(Context context, Intent intent) {
		String action = intent.getAction();
		if (SECRET_CODE_ACTION.equals(action)) {// SECRET_CODE_ACTION
			Intent i = new Intent(Intent.ACTION_MAIN);
			// i.setClass(context, sprd_engmode.class);
			// i.setClass(context, versioninfo.class);
			i.setClass(context, engineeringmodel.class);
			i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			context.startActivity(i);
		} else if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
			SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
			if (pref != null) {
				boolean exist = pref.contains(logswitch.KEY_CAPLOG);
				if (exist) {
					SharedPreferences.Editor editor = pref.edit();
					boolean v = pref.getBoolean(logswitch.KEY_CAPLOG, false);
					Log.d(TAG, "cap_log values : " + v);
					editor.putBoolean(logswitch.KEY_CAPLOG, false);
					editor.apply();
				}else{
					Log.d(TAG, "cap_log values not exist !");
				}
			}
		}
	}
}
