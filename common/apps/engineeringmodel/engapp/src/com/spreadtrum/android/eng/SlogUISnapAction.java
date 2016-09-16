package com.spreadtrum.android.eng;

import com.spreadtrum.android.eng.R;

import android.app.Activity;
import android.os.Bundle;
import android.widget.Toast;

/* SlogUI Added by Yuntao.xiao*/

public class SlogUISnapAction extends Activity {
    @Override
    protected void onCreate(Bundle snap) {
        super.onCreate(snap);
        //if (true)
        SlogAction.snap();
        finish();
        //
        //setTheme()
    }
}
