
package com.spreadtrum.android.eng;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.util.Log;

public class AppSettings extends PreferenceActivity {
    private static final String LOG_TAG = "engineeringmodel";

    private static final String CALL_FORWARD_QUERY = "call_forward_query";
    private static final String AUTO_RETRY_DIAL = "emergency_call_retry";

    private static final String CARD_LOG = "card_log";

    public static final String PREFS_NAME = "ENGINEERINGMODEL";

    private static final String AUTO_ANSWER = "autoanswer_call";
    private static final String ENABLE_VSER_GSER = "enable_vser_gser";  //add by liguxiang 07-12-11 for engineeringmoodel usb settings

    private static final String ENABLE_USB_FACTORY_MODE = "enable_usb_factory_mode";
    private static final String ACCELEROMETER = "accelerometer_rotation";

    private static final String MODEM_RESET = "modem_reset";

    private static final String ENG_TESTMODE = "engtestmode";

    private CheckBoxPreference mAutoAnswer;
    private CheckBoxPreference mEnableVserGser;  //add by liguxiang 07-12-11 for engineeringmoodel usb settings
    private CheckBoxPreference mAcceRotation;
    private CheckBoxPreference mEnableUsbFactoryMode;
    private CheckBoxPreference mModemReset;
    private EngSqlite mEngSqlite;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.layout.appset);

        mAutoAnswer = (CheckBoxPreference) findPreference(AUTO_ANSWER);
        mEnableVserGser = (CheckBoxPreference) findPreference(ENABLE_VSER_GSER);  //add by liguxiang 07-12-11 for engineeringmoodel usb settings
        mAcceRotation = (CheckBoxPreference)findPreference(ACCELEROMETER);
        mEnableUsbFactoryMode = (CheckBoxPreference)findPreference(ENABLE_USB_FACTORY_MODE);
        mModemReset = (CheckBoxPreference)findPreference(MODEM_RESET);

        String result = SystemProperties.get("persist.sys.sprd.modemreset");
        Log.e(LOG_TAG, "result: "+ result + ", result.equals(): " + (result.equals("1")));
        mModemReset.setChecked(result.equals("1"));
        mEngSqlite = EngSqlite.getInstance(this);
    }

    @Override
    protected void onResume() {
    	boolean check = Settings.System.getInt(getContentResolver(),Settings.System.ACCELEROMETER_ROTATION, 1) == 1;
    	if(check){
    		SystemProperties.set("persist.sys.acce_enable", check ? "1": "0");
    	}
		mAcceRotation.setChecked(check);
        String usbMode = SystemProperties.get("sys.usb.config", "");
        Log.e(LOG_TAG, " usbMode = " + usbMode);
        mEnableVserGser.setChecked(usbMode.endsWith("vser,gser"));
        boolean test = mEngSqlite.queryData(ENG_TESTMODE);
        if (!test) {
            mEnableUsbFactoryMode.setChecked(true);
        } else {
            int mode = mEngSqlite.queryFactoryModeDate(ENG_TESTMODE);
            mEnableUsbFactoryMode.setChecked(mode == 1);
        }
        
    	super.onStart();
    }

    @Override
    protected void onDestroy() {
        mEngSqlite.release();
        super.onDestroy();
    }

    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {

	if (preference == mAutoAnswer) {
			boolean newState = mAutoAnswer.isChecked();

			SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
			//boolean silent = settings.getBoolean("autoanswer_call", false);
			SharedPreferences.Editor editor = settings.edit();
			editor.putBoolean("autoanswer_call", newState);
			editor.commit();

			if (newState){
		      		getApplicationContext().startService(new Intent(getApplicationContext(), AutoAnswerService.class));
			}
			else{
				getApplicationContext().stopService(new Intent(getApplicationContext(), AutoAnswerService.class));
			}

			Log.e(LOG_TAG, "auto answer state "+newState);
			return true;
		//add by liguxiang 07-12-11 for engineeringmoodel usb settings begin
	}else if(preference == mEnableVserGser){
			boolean newState = mEnableVserGser.isChecked();
			Settings.Secure.putInt(getContentResolver(),
							Settings.Secure.VSER_GSER_ENABLED, newState ? 1 : 0);
			return true;
	}else if(preference == mAcceRotation){
			boolean checked = mAcceRotation.isChecked();
			//set accelerometer listener not be called in SensorManager#ListenerDelegate
			SystemProperties.set("persist.sys.acce_enable", checked ? "1": "0");
			//set Orientation of screen not be changed
			Settings.System.putInt(getContentResolver(),
					Settings.System.ACCELEROMETER_ROTATION, checked ? 1 : 0);
			return true;
	}
	else if(preference == mEnableUsbFactoryMode){
		boolean checked = mEnableUsbFactoryMode.isChecked();
		mEngSqlite.updataFactoryModeDB(ENG_TESTMODE,checked ? 1 : 0);
		return true;
	}
	//add by liguxiang 07-12-11 for engineeringmoodel usb settings end

        final String key = preference.getKey();

        if (CALL_FORWARD_QUERY.equals(key)) {
            SystemProperties.set("persist.sys.callforwarding", ((CheckBoxPreference) preference)
                    .isChecked() ? "1" : "0");
            return true;
        } else if (AUTO_RETRY_DIAL.equals(key)) {
            SystemProperties.set("persist.sys.emergencyCallRetry", ((CheckBoxPreference) preference)
                    .isChecked() ? "1" : "0");
            return true;
        } else if (CARD_LOG.equals(key)) {
            SystemProperties.set("persist.sys.cardlog", ((CheckBoxPreference) preference)
                    .isChecked() ? "1" : "0");
            return true;
        } else if ("modem_reset".equals(key)) {
	    SystemProperties.set("persist.sys.sprd.modemreset",
		((CheckBoxPreference) preference).isChecked() ? "1" : "0");
            return true;
	}else{
            return false;
        }

    }
}
