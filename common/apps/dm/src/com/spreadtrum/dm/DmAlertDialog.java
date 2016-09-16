
package com.spreadtrum.dm;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;
import android.content.SharedPreferences;
import android.content.Context;
import android.util.Log;
import android.content.Intent; //import com.android.dm.vdmc.VdmcFumoHandler;
//import com.android.dm.vdmc.MyConfirmation;
import android.app.AlertDialog;
import android.content.DialogInterface; //import com.redbend.vdm.*;
import android.app.Notification;
import android.app.NotificationManager;
import android.os.Handler;
import java.lang.Runnable;
import android.view.WindowManager;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.app.KeyguardManager;
import android.app.KeyguardManager.KeyguardLock;

import com.spreadtrum.dm.R;
import com.spreadtrum.dm.DmNativeInterface;
import com.spreadtrum.dm.vdmc.Vdmc;

import android.text.format.DateFormat;
import android.media.AudioManager;
import android.widget.Toast;

public class DmAlertDialog extends Activity {
    private String TAG = DmReceiver.DM_TAG + "DmAlertDialog: ";
    private String TAGCONF = "MyConfirmation";

    private Context mContext = null;

    private static DmAlertDialog mInstance = null;

    private NotificationManager mNotificationMgr = null;

    private WakeLock mWakelock = null;

    private int mDialogId = Vdmc.DM_NULL_DIALOG;

    private KeyguardLock mKeyguardLock = null;

    private boolean isScreenLock = false;

    private boolean isTimerActive = false;

    private Toast mToast = null;
	
    private AlertDialog builder = null;

    private Handler handler = new Handler();

    private Runnable runnable = new Runnable() {


	
        public void run() {
            stopTimer(); // stop timer

            // close alert dialog
            Log.d(TAG, "run() : mDialogId = " + mDialogId);
            switch (mDialogId) {
                case Vdmc.DM_NIA_INFO_DIALOG:
                    // DmJniInterface.startPppConnect();//start dial up
                    // DMNativeMethod.JVDM_notifyNIASessionProceed();

                   // notifyNIASessionProceed(); 2012-3-31@hong
		        DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);

                    builder.dismiss();

                    finish();
                    DestroyAlertDialog();

                    break;

                case Vdmc.DM_NIA_CONFIRM_DIALOG:
                    DmService.getInstance().getDmNativeInterface().spdm_jni_stopDm(DmNativeInterface.STOP_DM_REASON_DEFAULT);
                    // Vdmc.getInstance().stopVDM();
                    builder.dismiss();
                    finish();
                    DestroyAlertDialog();
                    break;

                case Vdmc.DM_ALERT_CONFIRM_DIALOG:
                    handleTimeoutEvent();
                    // MyConfirmation.getInstance().handleTimeoutEvent();
                    // cancel dm session
                    break;

                default:
                    Log.d(TAG, "run() : mDialogId is invalid ");
                    break;
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // setContentView(R.layout.main);
        // getWindow().setBackgroundDrawable(null);
        mContext = this;
        mInstance = this;
        mNotificationMgr = (NotificationManager) mContext
                .getSystemService(Context.NOTIFICATION_SERVICE);

        /*
         * //forbid sleep int flag =
         * WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON |
         * WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON |
         * WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED |
         * WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD;
         * getWindow().setFlags(flag, flag);
         * getWindow().setContentView(R.layout.alert_dialog);
         */

        getUnlock();

        Intent intent = getIntent();
	 int id = intent.getIntExtra("dialogId", Vdmc.DM_NULL_DIALOG);        
	 Log.d(TAG, "OnCreate: id = " + id);
        String msg = intent.getStringExtra("message");
	// Log.d(TAG, "OnCreate: msg = " + msg);
        int timeout = intent.getIntExtra("timeout", 60); // default 1min
         Log.d(TAG, "OnCreate: timeout = " + timeout);
        Log.d(TAG, "OnCreate: mContext = " + mContext);

      
        
       

        CreateAlertDialog(id, msg, timeout);
        mDialogId = id;
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy:");
        super.onDestroy();
        stopTimer(); // if switch to horizontal display, the window will auto
                     // close by system, and create new window, need stop timer
        if (mNotificationMgr != null) {
            mNotificationMgr.cancel(100);
            mNotificationMgr = null;
        }
        /*
         * mContext = null; mInstance = null; 
         * mNotificationMgr.cancel(100);
         * mNotificationMgr = null; 
         * stopTimer(); 
         * releaseUnlock();
         */
    }

    // do some reset operation
    // finish() is asynchronous function, onDestroy() sometimes comes too
    // late,will result error
    private void DestroyAlertDialog() {
        Log.d(TAG, "DestroyAlertDialog");
        mContext = null;
        mInstance = null;
        mNotificationMgr.cancel(100);
        mNotificationMgr = null;

        stopTimer();
        releaseUnlock();
	System.gc();
    }

    public static DmAlertDialog getInstance() {
        return mInstance;
    }

    public void finishDialog() {
        finish();
        DestroyAlertDialog();
    }

    private void getUnlock() {
        // acquire wake lock
        PowerManager pm = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        mWakelock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP
                | PowerManager.ON_AFTER_RELEASE, "SimpleTimer");
        mWakelock.acquire();
        Log.d(TAG, "getUnlock: acquire Wakelock");

        // unlock screen
        KeyguardManager km = (KeyguardManager) mContext.getSystemService(Context.KEYGUARD_SERVICE);
        mKeyguardLock = km.newKeyguardLock(TAG);
        if (km.inKeyguardRestrictedInputMode()) {
            Log.d(TAG, "getUnlock: disable keyguard");
            mKeyguardLock.disableKeyguard();
            isScreenLock = true;
        } else {
            isScreenLock = false;
        }
    }

    private void releaseUnlock() {
        // release screen
        if (isScreenLock) {
            Log.d(TAG, "releaseUnlock: reenable Keyguard");
            mKeyguardLock.reenableKeyguard();
            isScreenLock = false;
        }
        // release wake lock
        if (mWakelock.isHeld()) {
            Log.d(TAG, "releaseUnlock: release Wakelock");
            mWakelock.release();
        }
    }

    private void playAlertSound() {
        Log.d(TAG, "playAlertSound:");
        Notification n = new Notification();

        n.flags |= Notification.FLAG_ONLY_ALERT_ONCE;
        n.defaults = Notification.DEFAULT_SOUND;

        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        boolean nowSilent = audioManager.getRingerMode() != AudioManager.RINGER_MODE_NORMAL;
        if (nowSilent) {
            n.defaults |= Notification.DEFAULT_VIBRATE;
        }

        mNotificationMgr.notify(100, n);
    }

    private void startTimer(int timeout) // timeout : s
    {
        Log.d(TAG, "startTimer: timeout = " + timeout);

        // convert timout to ms
        isTimerActive = handler.postDelayed(runnable, timeout * 1000);// after timeout's operate runnable
    }

    private void stopTimer() {
        Log.d(TAG, "stopTimer: isTimerActive = " + isTimerActive);
        if (isTimerActive) {
            handler.removeCallbacks(runnable);
            isTimerActive = false;
        }
    }

    private void CreateAlertDialog(int id, String message, int timeout) {
        Log.d(TAG, "CreateAlertDialog: id = " + id);
        switch (id) {
            case Vdmc.DM_NULL_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_NULL_DIALOG");
                break;

            case Vdmc.DM_NIA_INFO_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_NIA_INFO_DIALOG");
/*				
		       if (mToast == null)
		            mToast = Toast.makeText(mContext,message,6000);
		        mToast.setText(message);
			 mToast.setDuration(6000);
			 playAlertSound();
		        mToast.show();
		DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);				
	        DmAlertDialog.getInstance().finishDialog();
*/	        
// @removed by Hong 2012-3-21
                String notify_msg = Vdmc.getAppContext().getResources().getString(
                        R.string.notify_msg);
                createNiaInformDialog(notify_msg, timeout);
//                
                break;

            case Vdmc.DM_NIA_CONFIRM_DIALOG:
                String notify_confirm_msg = Vdmc.getAppContext().getResources().getString(
                        R.string.notify_confirm_msg);
                Log.d(TAG, "CreateAlertDialog: DM_NIA_CONFIRM_DIALOG");
                createNiaConfirmDialog(notify_confirm_msg, timeout);
                break;

            case Vdmc.DM_ALERT_CONFIRM_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_ALERT_CONFIRM_DIALOG" + message);
                CharSequence strSou = "&amp;";
                CharSequence strDes = "&";
                message = message.replace(strSou, strDes);
                Log.d(TAG, "CreateAlertDialog: DM_ALERT_CONFIRM_DIALOG1" + message);
                createAlertConfirmDialog(message, timeout);
                break;

            case Vdmc.DM_CONFIRM_DOWNLOAD_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_CONFIRM_DOWNLOAD_DIALOG");
                // Vdmc.getFumoHandler().createDialog(mContext,
                // VdmcFumoHandler.CONFIRM_DOWNLOAD).show();
                break;

            case Vdmc.DM_CONFIRM_UPDATE_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_CONFIRM_UPDATE_DIALOG");
                // Vdmc.getFumoHandler().createDialog(mContext,
                // VdmcFumoHandler.CONFIRM_UPDATE).show();
                break;

            case Vdmc.DM_SIMULATE_UPDATE_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_CONFIRM_UPDATE_DIALOG");
                // Vdmc.getFumoHandler().createDialog(mContext,
                // VdmcFumoHandler.SIMULATE_UPDATE).show();
                break;

            case Vdmc.DM_PROGRESS_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_PROGRESS_DIALOG");
                break;

            default:
                break;
        }
    }

    private void createNiaInformDialog(String message, int timeout) {
        String softkey_ok = getResources().getString(R.string.menu_ok);

        playAlertSound();

        builder = new AlertDialog.Builder(mContext).setTitle("").setMessage(message)
                .setPositiveButton(softkey_ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.d(TAG, "createNiaInformDialog: onClick OK, notifyNIASessionProceed");
                        finish();
                        DestroyAlertDialog();
                        // start dial up
                        // DmJniInterface.startPppConnect();
                        // DMNativeMethod.JVDM_notifyNIASessionProceed();
                        DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);
                        
                        notifyNIASessionProceed();
                    }
                })
                .setOnCancelListener(new DialogInterface.OnCancelListener() {
                    public void onCancel(DialogInterface dialog) {
                        Log.d(TAG, "createNiaInformDialog: onCancel, notifyNIASessionProceed");
                        finish();
                        DestroyAlertDialog();
                        // start dial up
                        // DmJniInterface.startPppConnect();
                        // DMNativeMethod.JVDM_notifyNIASessionProceed();
                        DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);

                        notifyNIASessionProceed();
                    }
                })
                .show();
           startTimer(5);  //accord to cmcc, 2-10s 
//        startTimer(timeout);
    }

    private void createNiaConfirmDialog(String message, int timeout) {
        String softkey_yes = getResources().getString(R.string.menu_yes);
        String softkey_no = getResources().getString(R.string.menu_no);

        playAlertSound();

        builder = new AlertDialog.Builder(mContext).setTitle("").setMessage(message)
                .setPositiveButton(softkey_yes, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.d(TAG, "createNiaConfirmDialog: onClick Yes, notifyNIASessionProceed");
                        // start dial up
                        // DMNativeMethod.JVDM_notifyNIASessionProceed();
                        DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);
                        //notifyNIASessionProceed(); hong2012
                        finish();
                        DestroyAlertDialog();
                        // DmService.getInstance().startPppConnect();
                    }
                }).setNegativeButton(softkey_no, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.d(TAG, "createNiaConfirmDialog: onClick No, cancel dm session!");
                        //DmService.getInstance().getDmNativeInterface().spdm_jni_stopDm(DmNativeInterface.STOP_DM_REASON_DEFAULT);
                        // Vdmc.getInstance().stopVDM();
                       DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(false);
                        finish();
                        DestroyAlertDialog();
                    }
                }).setOnCancelListener(new DialogInterface.OnCancelListener() {
                    public void onCancel(DialogInterface dialog) {
                        Log.d(TAG, "createNiaConfirmDialog: onCancel, default cancel dm session!");
                        // Vdmc.getInstance().stopVDM();
                        DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(false);
//                        DmService.getInstance().getDmNativeInterface().spdm_jni_stopDm(DmNativeInterface.STOP_DM_REASON_DEFAULT);
                        finish();
                        DestroyAlertDialog();
                    }
                })
                .show();

        startTimer(timeout);
    }

    private void createAlertConfirmDialog(String message, int timeout) {
        playAlertSound();
        startTimer(timeout);
        String title = getResources().getString(R.string.notify_confirm_msg);
        String softkey_yes = getResources().getString(R.string.menu_yes);
        String softkey_no = getResources().getString(R.string.menu_no);
        createAlertConfirmDialog(mContext, title, message, softkey_yes, softkey_no, timeout);
    }

    private void createAlertConfirmDialog(Context context, String title, String message,
            String leftSfk, String rightSfk, int timeout)// lihui NIA before
                                                         // ALERT
    {
        builder = new AlertDialog.Builder(context).setTitle(title).setMessage(message)

        .setPositiveButton(leftSfk, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                Log.d(TAGCONF,
                        "createAlertConfirmDialog: onClick Yes, notifyConfirmationResult true");
                // observer.notifyConfirmationResult(true);
                DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);
                DmAlertDialog.getInstance().finishDialog();
            }
        }).setNegativeButton(rightSfk, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                Log.d(TAGCONF,
                        "createAlertConfirmDialog: onClick Yes, notifyConfirmationResult false");
                DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(false);
                // observer.notifyConfirmationResult(false);//lihui
                // hs_dm_mmi_confirmationQuerycb
                DmAlertDialog.getInstance().finishDialog();
            }
        }).setOnCancelListener(new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface dialog) {
                Log.d(TAGCONF,
                        "createAlertConfirmDialog: onClick Yes, notifyCancelEvent");
                // observer.notifyCancelEvent();
                DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(false);
                DmAlertDialog.getInstance().finishDialog();
            }
        })

        .show();
    }

    private void handleTimeoutEvent() {
        Log.d(TAGCONF, "handleTimeoutEvent: alert window timeout");
        // observer.notifyCancelEvent();
        // DmAlertDialog.getInstance().finishDialog();
        DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(false);
        builder.dismiss();
        DmAlertDialog.getInstance().finishDialog();
    }

    private void notifyNIASessionProceed() {
        Thread t = new Thread() {
            public void run() {
                Log.d(TAG, "Thread notifyNIASessionProceed ");
                Log.d(TAG, " DIAL START TIME = "
                        + DateFormat.format("yyyy-MM-dd kk:mm:ss", System.currentTimeMillis()));
                //TODO: to be confirm
//                DMNativeMethod.JVDM_notifyNIASessionProceed();
            }
        };
        t.start();
    }
}
