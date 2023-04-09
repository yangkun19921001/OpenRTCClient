package com.example.p2ps;

import android.content.Intent;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.GridLayout;

import org.webrtc.SurfaceViewRenderer;

public class MainActivity extends AppCompatActivity {


    public static String CONFIG_ROOM_URL = "room_url";
    public static String CONFIG_ROOM_ID = "room_id";

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 隐藏状态栏和导航栏
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_main);

    }

    public void join(View view) {
        EditText roomId = findViewById(R.id.room);
        EditText serverInfo = findViewById(R.id.server_info);
        Intent intent = new Intent(this, RTCRoomActivity.class);
        intent.putExtra(CONFIG_ROOM_ID, roomId.getText().toString().trim());
        intent.putExtra(CONFIG_ROOM_URL, serverInfo.getText().toString().trim());
        startActivity(intent);
    }
}