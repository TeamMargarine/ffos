package com.spreadtrum.android.eng;

import com.spreadtrum.android.eng.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.Toast;

import com.android.internal.app.IMediaContainerService;

public class LogSettingSlogUICommonControl extends Activity {
    private Button btnClear;
    private Button btnDump;
    private RadioButton rdoGeneralOn;
    private RadioButton rdoGeneralOff;
    private RadioButton rdoGeneralLowPower;
    private CheckBox chkAndroid;
    private CheckBox chkModem;
    private CheckBox chkAlwaysRun;
    private CheckBox chkSnap;
    private CheckBox chkClearLogAuto;
    private RadioButton rdoNAND;
    private RadioButton rdoSDCard;
    private Intent intentSvc;
    private Intent intentSnap;
    private Intent intentMedia;
    private IMediaContainerService mMediaContainer;
    private static final ComponentName DEFAULT_CONTAINER_COMPONENT = new ComponentName(
            "com.android.defcontainer", "com.android.defcontainer.DefaultContainerService");
    // Only to prepare to get freespace from sdcard because engmode is a system process.     
    final private ServiceConnection mMediaContainerConn = new ServiceConnection() {
        public void onServiceConnected(ComponentName name, IBinder service) {
            final IMediaContainerService imcs = IMediaContainerService.Stub
            .asInterface(service);
            Log.e("SlogUI","has this connection worked?");
            mMediaContainer = imcs;
        }

        public void onServiceDisconnected(ComponentName name) {
        }
    };
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_general_commoncontrol);

        // Init views
        rdoGeneralOn = (RadioButton) findViewById(R.id.rdo_general_on);
        rdoGeneralOff = (RadioButton) findViewById(R.id.rdo_general_off);
        rdoGeneralLowPower = (RadioButton) findViewById(R.id.rdo_general_lowpower);
        chkAndroid = (CheckBox) findViewById(R.id.chk_general_android_switch);
        chkModem = (CheckBox) findViewById(R.id.chk_general_modem_switch);
        chkAlwaysRun = (CheckBox) findViewById(R.id.chk_general_alwaysrun);
        chkSnap = (CheckBox) findViewById(R.id.chk_general_snapsvc);
        chkClearLogAuto = (CheckBox) findViewById(R.id.chk_general_autoclear);
        rdoNAND = (RadioButton) findViewById(R.id.rdo_general_nand);
        rdoSDCard = (RadioButton) findViewById(R.id.rdo_general_sdcard);
        btnClear = (Button) findViewById(R.id.btn_general_clearall);
        btnDump = (Button) findViewById(R.id.btn_general_dump);

        // Init intents
        intentSvc = new Intent("svcSlog");
        intentSvc.setClass(this, SlogService.class);
        intentSnap = new Intent("svcSnap");
        intentSnap.setClass(LogSettingSlogUICommonControl.this,
                SlogUISnapService.class);
        intentMedia = new Intent().setComponent(DEFAULT_CONTAINER_COMPONENT);
        boolean a = getApplicationContext().bindService(intentMedia, mMediaContainerConn, Context.BIND_AUTO_CREATE);
        Log.e("SlogUI","bindService is " + String.valueOf(a) );
        // Sync view's status
        SyncState();

        chkAlwaysRun.setChecked (SlogAction.isAlwaysRun (SlogAction.SERVICESLOG));
        chkSnap.setChecked (SlogAction.isAlwaysRun (SlogAction.SERVICESNAP));
        chkClearLogAuto.setChecked (SlogAction.GetState (SlogAction.CLEARLOGAUTOKEY));
        
        if (chkAlwaysRun.isChecked()) {
            startService(intentSvc);
        }
        if (chkSnap.isChecked()) {
            startService(intentSnap);
        }

        // Fetch onclick listenner
        ClkListenner clickListen = new ClkListenner();
        rdoGeneralOn.setOnClickListener(clickListen);
        rdoGeneralOff.setOnClickListener(clickListen);
        rdoGeneralLowPower.setOnClickListener(clickListen);
        chkAndroid.setOnClickListener(clickListen);
        chkModem.setOnClickListener(clickListen);
        rdoNAND.setOnClickListener(clickListen);
        rdoSDCard.setOnClickListener(clickListen);
        btnClear.setOnClickListener(clickListen);
        btnDump.setOnClickListener(clickListen);
        chkAlwaysRun.setOnClickListener(clickListen);
        chkSnap.setOnClickListener(clickListen);
        chkClearLogAuto.setOnClickListener(clickListen);

    }

    @Override
    protected void onResume() {
        super.onResume();
        SyncState();
    }

    protected void SyncState() {
        // Set checkbox status
        boolean tempHostOn = SlogAction.GetState(SlogAction.GENERALKEY, true).equals(SlogAction.GENERALON);
        boolean tempHostLowPower = SlogAction.GetState(SlogAction.GENERALKEY, true).equals(SlogAction.GENERALLOWPOWER);
//        Log.e("test","tempHostOn=" + String.valueOf(tempHostOn) + "  \ttempHostLowPower" + String.valueOf(tempHostLowPower));
        if (tempHostOn) {
            rdoGeneralOn.setChecked(true);
        } else if (tempHostLowPower) {
            rdoGeneralLowPower.setChecked(true);
        } else {
            rdoGeneralOff.setChecked(true);
        }
        boolean tempHost = tempHostOn || tempHostLowPower;
        SlogAction.SetCheckBoxBranchState(chkAndroid, tempHost,
                SlogAction.GetState(SlogAction.ANDROIDKEY));

        SlogAction.SetCheckBoxBranchState(chkModem, tempHost,
                SlogAction.GetState(SlogAction.MODEMKEY));

        // Set Radio buttons
        if (SlogAction.GetState(SlogAction.STORAGEKEY)) {
            if (SlogAction.IsHaveSDCard()) {
                rdoSDCard.setChecked(true);
                btnDump.setEnabled(true);
            } else {
                rdoNAND.setChecked(true);
                SlogAction.SetState(SlogAction.STORAGEKEY,
                        SlogAction.STORAGENAND, true);
                btnDump.setEnabled(false);
            }
        } else {
            rdoNAND.setChecked(true);
            btnDump.setEnabled(false);
        }
        rdoSDCard.setEnabled(SlogAction.IsHaveSDCard() ? true : false);

        // set clear all logs
        if (tempHost) {
            btnClear.setEnabled(false);
        } else {
            btnClear.setEnabled(true);
        }

    }

    protected class ClkListenner implements OnClickListener {

        public void onClick(View v) {

            switch (v.getId()) {
            case R.id.rdo_general_on:
                SlogAction.SetState(SlogAction.GENERALKEY,
                        SlogAction.GENERALON, true);
                SyncState();
                if (rdoGeneralOff.isChecked()) {
                    requestDumpLog();
                }
                break;
                
            case R.id.rdo_general_off:
                SlogAction.SetState(SlogAction.GENERALKEY,
                        SlogAction.GENERALOFF, true);
                SyncState();
            break;
            
            case R.id.rdo_general_lowpower:
                SlogAction.SetState(SlogAction.GENERALKEY,
                        SlogAction.GENERALLOWPOWER, true);
                SyncState();
            break;
            
            case R.id.chk_general_android_switch:
                SlogAction.SetState(SlogAction.ANDROIDKEY,
                        chkAndroid.isChecked());
                break;

            case R.id.chk_general_modem_switch:
                SlogAction.SetState(SlogAction.MODEMKEY, chkModem.isChecked(),
                        false);
                break;

            case R.id.chk_general_alwaysrun:
                SlogAction.setAlwaysRun(SlogAction.SERVICESLOG,
                        chkAlwaysRun.isChecked());
                if (chkAlwaysRun.isChecked()) {
                    // start slog service
                    if (startService(intentSvc) == null) {
                        Toast.makeText(
                                LogSettingSlogUICommonControl.this,
                                getText(R.string.toast_service_slog_start_failed),
                                Toast.LENGTH_SHORT).show();
                    }
                } else {
                    // stop slog service
                    if (!stopService(intentSvc)) {
                        Toast.makeText(
                                LogSettingSlogUICommonControl.this,
                                getText(R.string.toast_service_slog_end_failed),
                                Toast.LENGTH_SHORT).show();
                    }
                }
                break;

            case R.id.chk_general_snapsvc:
                if (chkSnap.isChecked()) {
                    // start snap service
                    Toast.makeText(LogSettingSlogUICommonControl.this,
                            getText(R.string.toast_snap_prompt),
                            Toast.LENGTH_SHORT).show();
                    if (startService(intentSnap) == null) {
                        Toast.makeText(
                                LogSettingSlogUICommonControl.this,
                                getText(R.string.toast_service_snap_start_failed),
                                Toast.LENGTH_SHORT).show();
                    }
                } else {
                    // stop snap service
                    if (!stopService(intentSnap)) {
                        Toast.makeText(
                                LogSettingSlogUICommonControl.this,
                                getText(R.string.toast_service_snap_end_failed),
                                Toast.LENGTH_SHORT).show();
                    }
                }
                SlogAction.setAlwaysRun(SlogAction.SERVICESNAP,
                        chkSnap.isChecked());
                break;

            case R.id.chk_general_autoclear:
                SlogAction.SetState (SlogAction.CLEARLOGAUTOKEY, chkClearLogAuto.isChecked(), true);
                break;

            case R.id.rdo_general_nand:
                Toast.makeText(
                        LogSettingSlogUICommonControl.this,
                        getText(R.string.toast_freespace_nand)
                                + String.valueOf(SlogAction
                                        .GetFreeSpace(mMediaContainer, SlogAction.STORAGENAND))
                                + "MB", Toast.LENGTH_SHORT).show();
                SlogAction.SetState(SlogAction.STORAGEKEY,
                        rdoSDCard.isChecked(), true);
                btnDump.setEnabled(false);
                break;
            case R.id.rdo_general_sdcard:
                if (SlogAction.IsHaveSDCard()) {
                    Toast.makeText(
                            LogSettingSlogUICommonControl.this,
                            getText(R.string.toast_freespace_sdcard)
                                    + String.valueOf(SlogAction
                                            .GetFreeSpace(mMediaContainer, SlogAction.STORAGESDCARD))
                                    + "MB", Toast.LENGTH_SHORT).show();
                }
                SlogAction.SetState(SlogAction.STORAGEKEY,
                        rdoSDCard.isChecked(), true);
                btnDump.setEnabled(true);
                break;

            case R.id.btn_general_clearall:
                clearLog();
                break;

            case R.id.btn_general_dump:
                dump();
                break;
            }

        }
    }

    void dump() {

        final EditText edtDump = new EditText(
                LogSettingSlogUICommonControl.this);
        if (edtDump == null) {
            return ;
        }
        edtDump.setSingleLine(true);
        new AlertDialog.Builder(LogSettingSlogUICommonControl.this)
                //
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setTitle(R.string.alert_dump_title)
                .setView(edtDump)
                .setPositiveButton(R.string.alert_dump_dialog_ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int whichButton) {
                                if (null == edtDump.getText()) {
                                   return;
                                }
                                String fileName = edtDump.getText().toString();
                                java.util.regex.Pattern pattern = java.util.regex.Pattern.compile("[0-9a-zA-Z]*");
                                if (pattern.matcher(fileName).matches() && !"".equals(fileName)) {
                                    SlogAction.Dump(edtDump.getText()
                                        .toString());
                                } else {
                                    Toast.makeText(LogSettingSlogUICommonControl.this
                                                , getText(R.string.toast_dump_filename_error)
                                                , Toast.LENGTH_LONG)
                                            .show();
                                }
                                    /* User clicked OK so do some stuff */
                        }
                })
                .setNegativeButton(R.string.alert_dump_dialog_cancel,
                    new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int whichButton) {

                                /* User clicked cancel so do some stuff */
                            }
                        }).create().show();
        }

        void clearLog() {

            new AlertDialog.Builder(LogSettingSlogUICommonControl.this)
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setTitle(R.string.alert_clear_title)
                    .setMessage(R.string.alert_clear_string)
                    .setPositiveButton(R.string.alert_clear_dialog_ok,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int whichButton) {
                                    SlogAction.ClearLog();
                                    /* User clicked OK so do some stuff */
                                }
                            })
                    .setNegativeButton(R.string.alert_clear_dialog_cancel, null)
                    .create().show();

        }

    @Override
    protected Dialog onCreateDialog(int id) {
        return super.onCreateDialog(id);
    }

    void requestDumpLog() {
        new AlertDialog.Builder(LogSettingSlogUICommonControl.this)
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setTitle(R.string.alert_request_dump_title)
                .setMessage(R.string.alert_request_dump_prompt)
                .setPositiveButton(R.string.alert_dump_dialog_ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int whichButton) {
                                if (rdoSDCard.isChecked()) {
                                    dump();
                                }
                            }
                        })
                .setNegativeButton(R.string.alert_dump_dialog_cancel, null)
                .create().show();
    }

    // Resolve dialog missing when orientation start.
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

}
