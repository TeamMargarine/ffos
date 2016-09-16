package com.spreadtrum.android.eng;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.SystemProperties;
import android.preference.PreferenceManager;
import android.util.Log;

public class AutoAnswerReceiver extends BroadcastReceiver {
	private final String TAG = "AutoAnswerReceiver";

	public static final String PREFS_NAME = "ENGINEERINGMODEL";

	@Override
	public void onReceive(Context context, Intent intent) {
		if (intent.getAction().equals(Intent.ACTION_BOOT_COMPLETED)) {
			SharedPreferences settings = context.getSharedPreferences(
					PREFS_NAME, 0);
			boolean is_answer = settings.getBoolean("autoanswer_call", false);

			Log.e(TAG, "start AutoAnswerService being" + is_answer);

			if (is_answer) {
				Log.e(TAG, "start AutoAnswerService");
				context.startService(new Intent(context,
						AutoAnswerService.class));
			}
			// add by wangxiaobin 11-9 for cmmb set begin
			SharedPreferences defaultSettings = PreferenceManager
					.getDefaultSharedPreferences(context);
			boolean testIsOn = defaultSettings.getBoolean(
					CMMBSettings.TEST_MODE, false);
			boolean wireTestIsOn = defaultSettings.getBoolean(
					CMMBSettings.WIRE_TEST_MODE, false);
			if (testIsOn) {
				SystemProperties.set("ro.hisense.cmcc.test", "1");
			} else {
				SystemProperties.set("ro.hisense.cmcc.test", "0");
			}
			if (wireTestIsOn) {
				SystemProperties.set("ro.hisense.cmcc.test.cmmb.wire", "1");
			} else {
				SystemProperties.set("ro.hisense.cmcc.test.cmmb.wire", "0");
			}
			// add by wangxiaobin 11-9 cmmb set end
		}
	}
}
