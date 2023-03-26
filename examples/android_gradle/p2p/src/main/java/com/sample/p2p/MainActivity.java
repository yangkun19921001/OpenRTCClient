package com.sample.p2p;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.EditText;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/3/17
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
public class MainActivity extends AppCompatActivity {


    public static String CONFIG_ROOM_URL = "room_url";
    public static String CONFIG_ROOM_ID = "room_id";
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

    }

    public void join(View view) {
        EditText roomId = findViewById(R.id.room);
        EditText serverInfo = findViewById(R.id.server_info);
        Intent intent = new Intent(this, P2PCallActivity.class);
        intent.putExtra(CONFIG_ROOM_ID, roomId.getText().toString().trim());
        intent.putExtra(CONFIG_ROOM_URL, serverInfo.getText().toString().trim());
        startActivity(intent);
    }
}
