package com.spreadtrum.android.eng;

import java.io.IOException;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;

import org.apache.http.util.EncodingUtils;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.os.StatFs;
import android.util.Log;
import android.widget.CheckBox;
import android.widget.RadioButton;

import android.os.storage.IMountService;
import com.android.internal.app.IMediaContainerService;
//import android.os.storage.StorageVolume;

public class SlogAction {
    // =========================Const=============================================================================
    // slog.conf StorageLocation
    private static final String SLOG_CONF_LOCATION = "/data/local/tmp/slog/slog.conf";
    // Command to run slog
    private static final String SLOG_COMMAND_START = "slog";
    // A tester to confirm slog is running.
    // private static final String SLOG_COMMAND_QUERY = "slogctl query";
    // Run slog control after set States
    private static final String SLOG_COMMAND_RESTART = "slogctl reload";
    // Command : Clear Log
    private static final String SLOG_COMMAND_CLEAR = "slogctl clear";
    // Command : Export Logs
    private static final String SLOG_COMMAND_DUMP = "slogctl dump ";

    private static final String SLOG_COMMAND_SCREENSHOT = "slogctl screen";
    //
    private static final String APPFILES = "/data/data/com.spreadtrum.android.eng/files/";
    // New Feature
    // public static String Options[] = null;

    /** Tags,which used to differ Stream States On->true **/
    public static final String ON = "on";
    /** Tags,which used to differ Stream States Off->false **/
    public static final String OFF = "off";

    /** Log's StorageLocation SDCard->external or NAND->internal **/
    public static final String STORAGEKEY = "logpath\t";
    public static final String STORAGENAND = "internal";
    public static final String STORAGESDCARD = "external";

    /** keyName->General **/
    public static final String GENERALKEY = "\n";
    public static final String GENERALON = "enable";
    public static final String GENERALOFF = "disable";
    public static final String GENERALLOWPOWER = "low_power";

    /** keyName->Options **/
    public static final String KERNELKEY = "stream\tkernel\t";
    public static final String SYSTEMKEY = "stream\tsystem\t";
    public static final String RADIOKEY = "stream\tradio\t";
    public static final String MODEMKEY = "stream\tmodem\t";
    public static final String MAINKEY = "stream\tmain\t";
    public static final String TCPKEY = "stream\ttcp\t";
    public static final String BULETOOTHKEY = "stream\tbt\t";
    public static final String MISCKEY = "misc\tmisc\t";
    public static final String CLEARLOGAUTOKEY = "var\tslogsaveall\t";

    /** Android keyName **/
    public static final int ANDROIDKEY = 101;
    public static final String SERVICESLOG = "slogsvc.conf";
    public static final String SERVICESNAP = "snapsvc.conf";
    public static final String ANDROIDSPEC = "android";
    // new feature
    // private static final int LENGTHOPTIONSTREAM = 3;

    // public static final int OptionStreamState = 0;
    // public static final int OptionStreamSize = 1;
    // public static final int OptionStreamLevel = 2;

    // public static final String SIZE[] = new
    // String[]{"0","50","100","150","200"};

    // public static int position;
    private static final String ACTIONFAILED = "Failed";
    public static Context contextMainActivity;

    public static final int MESSAGE_START_READING = 11;
    public static final int MESSAGE_END_READING = 12;
    public static final int MESSAGE_START_WRITTING = 13;
    public static final int MESSAGE_END_WRITTING = 14;
    public static final int MESSAGE_START_RUN = 15;
    public static final int MESSAGE_END_RUN = 16;
    public static final int MESSAGE_DUMP_START = 17;
    public static final int MESSAGE_DUMP_STOP = 18;
    public static final int MESSAGE_CLEAR_START = 19;
    public static final int MESSAGE_CLEAR_END = 20;
    public static final int MESSAGE_SNAP_SUCCESSED = 21;
    public static final int MESSAGE_SNAP_FAILED = 22;

    public static String tester;

    // Temp Solution.
    private static boolean MMC_SUPPORT = "1".equals(android.os.SystemProperties.get("ro.device.support.mmc"));

    public static Handler handleMan;

    // ========================================================================================================

    // ==================================GetState=================================================

    /* Old Feature---------------------> */
    /**
     * Reload GetState Function, get states and compared, finally return the
     * result. PS:SDCard isChecked =>true
     **/
    public static boolean GetState(String keyName) {
        // null pointer handler
        if (keyName == null) {
            Log.e("slog",
                    "You have give a null to GetState():boolean,please check");
            return false;
        }

        try {
            if (GetState(keyName, false).equals(ON))
                return true;
            if (GetState(keyName, true).equals(ON))
                return true;
            if (keyName.equals(GENERALKEY)
                    && GetState(keyName, true).equals(GENERALON))
                return true;
            if (keyName.equals(STORAGEKEY)
                    && GetState(keyName, true).equals(STORAGESDCARD))
                return true;
        } catch (NullPointerException nullPointer) {
            Log.e("GetState",
                    "Maybe you change GetState(),but don't return null.Log:\n"
                            + nullPointer);
            return false;
        }
        return false;
    }

    /** Reload GetState function, for other condition **/
    public static boolean GetState(int otherCase) {
        // TODO if you have another Conditions, please use it,add the code under
        // switch with case
        // !
        try {
            switch (otherCase) {
            case ANDROIDKEY:
                if (GetState(KERNELKEY, false).equals(ON))
                    return true;
                else if (GetState(SYSTEMKEY, false).equals(ON))
                    return true;
                else if (GetState(RADIOKEY, false).equals(ON))
                    return true;
                else if (GetState(MAINKEY, false).equals(ON))
                    return true;
                break;
            default:
                Log.w("GetState(int)", "You have given a invalid case");
                break;
            }
        } catch (NullPointerException nullPointer) {
            Log.e("GetState(int)",
                    "Maybe you change GetState(),but don't return null.Log:\n"
                            + nullPointer);
            return false;
        }
        return false;
    }

    /**
     * Finally, we'll run the this GetState. It will return a result(String)
     * which you want to search after "keyName"
     **/
    public static String GetState(String keyName, boolean isLastOption) {
        // recieve all text from slog.conf
        StringBuilder conf = null;
        FileInputStream freader = null;
        //
        char result[] = null;
        try {
            // Create a fileInputStream with file-location
            freader = new FileInputStream(SLOG_CONF_LOCATION);

            // Dim a byte[] to get file we read.
            byte[] buffer = new byte[freader.available()];
            result = new char[freader.available()];
            // Begin reading
            try {
                freader.read(buffer);
                // Decoding
                conf = new StringBuilder(EncodingUtils.getString(buffer,
                        "UTF-8"));

            } catch (Exception e) {
                // Although I'm dead, I close it!
                freader.close();
                Log.e("GetStates->Readfile", e.toString());
                return ACTIONFAILED;
            }
            freader.close();

        } catch (Exception e) {
            Log.e("GetState(String,boolean):String", e.toString());
            return ACTIONFAILED;
        }

        conf.getChars(
                conf.indexOf(keyName) + keyName.length(), // start cursor
                conf.indexOf(isLastOption ? "\n" : "\t", conf.indexOf(keyName)
                        + keyName.length() + 1), // ending cursor
                result, 0);//

        return String.valueOf(result).trim();
    }

    /* <----------------------Old Feature */

    /*
     * New Feature----------------------> public static void GetStates(String
     * keyName){ int maxLength =
     * (keyName.equals(GENERALKEY)||keyName.equals(STORAGEKEY
     * ))?1:LENGTHOPTIONSTREAM; Options = new String[maxLength];
     *
     * StringBuilder conf = null; char result[] = new char[20];
     *
     * try{ FileInputStream freader = new FileInputStream(SLOG_CONF_LOCATION);
     * byte[] buffer = new byte[freader.available()]; freader.read(buffer); conf
     * = new StringBuilder(EncodingUtils.getString(buffer, "UTF-8"));
     * freader.close();
     *
     * } catch (Exception e) {
     * System.err.println("--->   GetState has problems, logs are followed:<---"
     * ); System.err.println(e); return ; } int counter = conf.indexOf(keyName);
     * int jump = keyName.length();
     *
     * for(int i=0 ;i<maxLength;){ conf.getChars( counter+jump,
     * conf.indexOf(i==maxLength-1?"\n":"\t",counter+jump+1), result, 0);
     *
     * //
     * System.out.println("i="+i+" counter="+counter+"jump="+jump+"result="+String
     * .valueOf(result).trim()); Options[i] = String.valueOf(result).trim();
     * counter += jump; jump = Options[i++].length()+1;
     *
     * } } /*<-----------------------New Feature
     */

    // =========================================================================================================GetState

    // ===============================SetState===============================================================

    /***/
    public static void SetState(String keyName, boolean status,
            boolean isLastOption) {
        if (keyName == null) {
            Log.e("SetState(String,boolean,boolean):void",
                    "Do NOT give me null");
            return;
        }
        // load files, and set the option
        if (keyName.equals(GENERALKEY)) {
            if (status) {
                SetState(keyName, GENERALON, true);
            } else {
                SetState(keyName, GENERALOFF, true);
            }
        } else if (keyName.equals(STORAGEKEY)) {
            if (status) {
                SetState(keyName, STORAGESDCARD, true);
            } else {
                SetState(keyName, STORAGENAND, true);
            }
        } else {
            SetState(keyName, status ? ON : OFF, isLastOption);
        }

        // RunThread runit = new RunThread();
        // new Thread(runit).start();

    }

    /** handle other case **/
    public static void SetState(int otherCase, boolean status) {
        // TODO if you have otherCondition, please add the code under switch
        // with case
        switch (otherCase) {
        case ANDROIDKEY:
            Message msg = new Message();
            msg.what = ANDROIDKEY;
            LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
            SetState(SYSTEMKEY, status, false);
            SetState(KERNELKEY, status, false);
            SetState(RADIOKEY, status, false);
            SetState(MAINKEY, status, false);
            break;
        default:
            Log.w("SetState(int,boolean)", "You have given a invalid case");
        }
    }

    public static void SetState(String keyName, String status,
            boolean isLastOption) {
        final String MethodName = "SetState(String,String,boolean):void";
        if (keyName == null) {
            Log.e(MethodName, "Do NOT give keyName null");
            return;
        }
        if (status == null) {
            Log.e(MethodName, "Do NOT give status null");
            return;
        }
        if (LogSettingSlogUITabHostActivity.mTabHostHandler != null) {
            Message msg = new Message();
            msg.what = MESSAGE_START_RUN;
            LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
        }

        // load files, and set the option
        try {
            StringBuilder conf = null;

            FileInputStream freader = new FileInputStream(SLOG_CONF_LOCATION);
            byte[] buffer = new byte[freader.available()];
            try {// Make sure=============================
                freader.read(buffer);
                freader.close();
            } catch (Exception e) {
                // ==============If dead,close file first.
                freader.close();
                Log.e(MethodName, "Reading file failed,now close,log:\n" + e);
                return;
            }
            conf = new StringBuilder(EncodingUtils.getString(buffer, "UTF-8"));
            int searchCursor = conf.indexOf(keyName);
            int keyNameLength = keyName.length();
            // ==================Judge Complete
            conf.replace(
                    searchCursor + keyNameLength,
                    conf.indexOf(isLastOption ? "\n" : "\t", searchCursor
                            + keyNameLength + 1), status);
            buffer = null;

            FileOutputStream fwriter = new FileOutputStream(SLOG_CONF_LOCATION);

            buffer = conf.toString().getBytes("UTF-8");
            try {
                fwriter.write(buffer);
                fwriter.close();
            } catch (Exception e) {
                fwriter.close();
                Log.e(MethodName, "Writing file failed,now close,log:\n" + e);
                return;
            }
        } catch (Exception e) {

            e.printStackTrace();
            Log.e(MethodName, "Catch Excepton,log:\n" + e);
            return;
        }

        RunThread runCommand = new RunThread();
        Thread runThread = new Thread(null, runCommand, "RunThread");
        runThread.start();
    }

    // ====================================================================================================SetState

    // Temp Solution for MMC
    
    private static final File EXTERNAL_STORAGE_DIRECTORY
            = getDirectory(getMainStoragePath(), "/mnt/sdcard/");

    private static String getExternalStorageState() {
        try {
            IMountService mountService = IMountService.Stub.asInterface(android.os.ServiceManager
                    .getService("mount"));
            return mountService.getVolumeState(EXTERNAL_STORAGE_DIRECTORY
                    .toString());
        } catch (Exception rex) {
            return "removed";
        }
    }

    private static String getMainStoragePath() {
        try {
            switch (Integer.parseInt(System.getenv("SECOND_STORAGE_TYPE"))) {
                case 0:
                    return "EXTERNAL_STORAGE";
                case 1:
                    return "EXTERNAL_STORAGE";
                case 2:
                    return "SECONDARY_STORAGE";
                default:
                    Log.e("SlogUI","Please check \"SECOND_STORAGE_TYPE\" "
                         + "\'S value after parse to int in System.getenv for framework");
                    if (MMC_SUPPORT) {
                        return "SECONDARY_SOTRAGE";
                    }
                    return "EXTERNAL_STORAGE";
            }
        } catch (Exception parseError) {
            Log.e("SlogUI","Parsing SECOND_STORAGE_TYPE crashed.\n" + parseError );
            if (MMC_SUPPORT) {
                return "SECONDARY_SOTRAGE";
            }
            return "EXTERNAL_STORAGE";
        }

    }

    private static File getDirectory(String variableName, String defaultPath) {
        String path = System.getenv(variableName);
        return path == null ? new File(defaultPath) : new File(path);
    }
    private static File getExternalStorage() {
        return EXTERNAL_STORAGE_DIRECTORY;
    }
    // Get it from framework ===============================================================================

    /** Make sure that SDCard is mounted **/
    public static boolean IsHaveSDCard() {
        if (android.os.Environment.getExternalStorageState() == null || getExternalStorageState() == null) {
            Log.e("SlogUI.isHaveSDCard():boolean",
                    "Your enviroment has something wrong,please check.\nReason:android.os.Environment.getExternalStorageState()==null");
            return false;
        }
//        if (MMC_SUPPORT) {
//            return getExternalStorageState().equals("mounted");
//        }
        return getExternalStorageState().equals(
                "mounted");
    }

    public static boolean IsSDCardSelected(RadioButton radioButton) {
        if (radioButton == null) {
            Log.e("IsSDCardSelected", "Do NOT    give RadioButton null");
            return false;
        }
        if (IsHaveSDCard()) {
            if (radioButton.isChecked()) {
                return true;
            }
        }
        return false;
    }

    /** Get free space **/
    public static long GetFreeSpace(IMediaContainerService imcs, String storageLocation) {
        // Judge null==========
        final String MethodName = "GetFreeSpace(String):long";
        if (android.os.Environment.getExternalStorageDirectory() == null
                || android.os.Environment.getDataDirectory() == null
                || EXTERNAL_STORAGE_DIRECTORY == null) {
            Log.e(MethodName, "Your environment has problem,please check");
            return 0;
        }
        if (storageLocation == null) {
            Log.e(MethodName, "Do NOT give storageLocation null");
            return 0;
        }
        // ========Judge Complete

        File path;
        long size = 0;
        // give location=========
        if (storageLocation.equals(STORAGESDCARD)) {
            path =  EXTERNAL_STORAGE_DIRECTORY;
            if (path != null) {
                try {
                    size = imcs.getFileSystemStats(path.getPath())[1];
                } catch (Exception e) {
                    if (imcs == null) {
                        Log.e("SlogUI","mediaContainerService is null");
                    } else {
                        Log.e("SlogUI","failed calculateDirectorySize(\""+path.getPath()+"\")");
                    }
                }
            }
        } else {
            path = android.os.Environment.getDataDirectory();
            if (path != null) {
                StatFs statFs = new StatFs(path.getPath());
                long blockSize = statFs.getBlockSize();
                long availableBlocks = statFs.getAvailableBlocks();
                size = availableBlocks * blockSize;
            }
        }

        return size / 1024 / 1024;
    }

    public static void SetCheckBoxBranchState(CheckBox tempCheckBox,
            boolean tempHost, boolean tempBranch) {
        // judge null
        final String MethodName = "SetCheckBoxBranchState(CheckBox,boolean,boolean):void";
        if (tempCheckBox == null) {
            Log.e(MethodName, "Do NOT give checkbox null");
            return;
        }
        // =========judge complete

        if (tempHost) {
            tempCheckBox.setEnabled(tempHost);
            tempCheckBox.setChecked(tempBranch);
        } else {
            tempCheckBox.setEnabled(tempHost);
        }
    }


    /** **/
    public static boolean isAlwaysRun(String keyName) {
        if (keyName == null) {
            return false;

        }
        if (keyName.equals(SERVICESLOG) || keyName.equals(SERVICESNAP)) {

        } else {
            return false;
        }

        byte[] buffer;
        String conf;
        FileInputStream freader = null;
        try {
            freader = new FileInputStream(APPFILES + keyName);

            buffer = new byte[freader.available()];
            freader.read(buffer);
            freader.close();
            conf = new String(EncodingUtils.getString(buffer, "UTF-8"));
            if (conf.trim().equals(String.valueOf(true))) {
                return true;
            }
            return false;

        } catch (Exception e) {
            if(freader!=null){
                try {
                    freader.close();
                } catch(IOException ioException){
                    Log.e("isAlwaysRun(String):boolean","Freader close error"+ioException);
                    return false;
                }
        }
            System.err.println("Maybe it is first Run,now try to create one.\n"+e);
	    FileOutputStream fwriter = null;
            try {
                fwriter = contextMainActivity.openFileOutput(
                        keyName, Context.MODE_PRIVATE);
                fwriter.write(String.valueOf(false).toString()
                        .getBytes("UTF-8"));
                fwriter.close();
            } catch (Exception e1) {
                System.err.println("No!! Create file failed, logs followed\n"
                        + e1);
		if(fwriter!=null) {
		    try {
		        fwriter.close();
		    } catch (IOException e2) {
		        return false;
		   }
		}
                return false;
            }
            System.err
                    .println("--->SetAlways Run failed,logs are followed:<---");
            System.err.println(e);
            return false;
        }
    }

    /** Make SlogService run in foreground all the time **/
    public static void setAlwaysRun(String keyService, boolean isChecked) {
	FileOutputStream fwriter = null;
        try {
            fwriter = contextMainActivity.openFileOutput(
                    keyService, Context.MODE_PRIVATE);
            fwriter.write(String.valueOf(isChecked).toString()
                    .getBytes("UTF-8"));
            fwriter.close();
        } catch (Exception e1) {
            Log.e("SetAlwaysRun", "Write file failed, see logs\n" + e1);
            return;
        } finally {
            if (fwriter != null) {
                try {
                    fwriter.close();
                } catch (IOException e1) {
                    Log.e("SetAlwaysRun(String,boolean):void","try to close fwriter failed"+e1);
                }
            }
        }
    }

    /** Clear Log **/
    public static void ClearLog() {
        Message msg = new Message();
        msg.what = MESSAGE_CLEAR_START;
        LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);

        ClearThread clearCommand = new ClearThread();
        Thread clearThread = new Thread(null, clearCommand, "clearThread");
        clearThread.start();

    }

    private static void runClearLog(){
        Message msg = new Message();
        msg.what = MESSAGE_CLEAR_END;
        try {
            Runtime runtime = Runtime.getRuntime();
            Process proc = runtime.exec(SLOG_COMMAND_CLEAR);

            try {
                if (proc.waitFor() != 0) {
                    Log.w("ClearLog", "Command" + SLOG_COMMAND_CLEAR
                            + " return value = " + proc.exitValue()
                            + ", maybe something wrong.");
                }
            } catch (InterruptedException e) {
                Log.e("ClearLog", e.toString());
                LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
                return;
            }
        } catch (Exception e) {
            LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
            return;
        }
        LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
    }


    /** Make logs into package. **/
    private static void runDump(String filename) {
        final String NowMethodName = "SlogUIDump";
        Message msg = new Message();
        msg.what = MESSAGE_DUMP_STOP;

        if (filename == null) {
            Log.e(NowMethodName, "Do NOT give me null");
            LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
            return;
        }

        try {
            Process proc = Runtime.getRuntime().exec(
                    SLOG_COMMAND_DUMP + filename);
            try {
                if (proc.waitFor() != 0) {
                    Log.w(NowMethodName, "Command" + SLOG_COMMAND_DUMP
                            + " exit value=" + proc.exitValue()
                            + ",maybe it has some problem");
                }
            } catch (InterruptedException e) {
                Log.e(NowMethodName, "InterruptedException->" + e);
            }

        } catch (Exception e) {
            Log.e(NowMethodName, e.toString());
            LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
            return;
        }
        LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
    }

    public static void Dump(String filename){
        if(filename==null){
            Log.e("SlogUIDump()", "Do not give nulll");
            return ;
        }
        Message msg = new Message();
        msg.what = MESSAGE_DUMP_START;
        LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
        DumpThread dumpCommand = new DumpThread(filename);
        Thread runThread = new Thread(null, dumpCommand, "DumpThread");
        runThread.start();
    }

    public static void SlogStart() {
        /*Modify 20130121 Spreadst 119078 engmode crash when cancell airplane mode start */
        Thread t =new Thread(){
            @Override
            public void run() {
                // TODO Auto-generated method stub
                Runtime runtime = Runtime.getRuntime();
                Process proc;
                try {
                    proc = runtime.exec(SLOG_COMMAND_START);
                    if (proc.waitFor() != 0) {
                        System.err.println("Exit value=" + proc.exitValue());
                    }
                } catch (IOException e1) {
                    // TODO Auto-generated catch block
                    e1.printStackTrace();
                }catch (InterruptedException e) {
                    System.err.println(e);
                }
            }
        };
        t.start();
        /*Modify 20130121 Spreadst 119078 engmode crash when cancell airplane mode end */
    }

    private static void runCommand() {
        Message msg = new Message();
        msg.what = MESSAGE_END_RUN;

        // Make sure that, users can not touch the ui many times
        try {
            Thread.sleep(200);
        } catch (InterruptedException e1) {
            e1.printStackTrace();
        }

        try {
            Runtime runtime = Runtime.getRuntime();
            Process proc = runtime.exec(SLOG_COMMAND_RESTART);
            try {
                if (proc.waitFor() != 0) {
                    System.err.println("Exit value=" + proc.exitValue());
                }
            } catch (InterruptedException e) {
                System.err.println(e);
            }

        } catch (Exception e) {
            Log.e("run", SLOG_COMMAND_RESTART
                    + " has Exception, log followed\n" + e);

            if (LogSettingSlogUITabHostActivity.mTabHostHandler != null) {
                LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
            }
            return;
        }
        if (LogSettingSlogUITabHostActivity.mTabHostHandler != null) {
            LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
        }
        return;

    }

    public static void snap() {
        Thread snapThread = new Thread() {
            @Override
            public void run() {
                Message msg = new Message();
                try {
                    Thread.sleep(500);
                    Runtime runtime = Runtime.getRuntime();
                    Process proc = runtime.exec(SLOG_COMMAND_SCREENSHOT);
                    try {
                        if (proc.waitFor() != 0) {
                        Log.i("Snap", "Exit value=" + proc.exitValue()
                            + ".Maybe not correct");
                        }

                        msg.what = MESSAGE_SNAP_SUCCESSED;
                        LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
                    } catch (InterruptedException e) {
                        msg.what = MESSAGE_SNAP_FAILED;
                        LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
                        Log.e("Snap", e.toString());
                    }

                } catch (Exception e) {
                    System.err.println(SLOG_COMMAND_RESTART
                        + " has Exception, log followed");
                    msg.what = MESSAGE_SNAP_FAILED;
                    LogSettingSlogUITabHostActivity.mTabHostHandler.sendMessage(msg);
                    return ;
                }
            }
        };
        snapThread.start();
        return ;
    }

    private static class RunThread implements Runnable {

        public void run() {
            runCommand();

        }

    }

    private static class DumpThread implements Runnable {
        String filename;
        public DumpThread(String fname) {
            super();
            filename = fname;
        }
        public void run() {
            if(filename==null){
                Message msg = new Message();
                msg.what = MESSAGE_DUMP_STOP;
                Log.e("SlogUIDumpThreadRun()", "filename==null");
                return;
            }
            runDump(filename);

        }
    }
    private static class ClearThread implements Runnable{
        public void run(){
            runClearLog();
        }
    }

}
