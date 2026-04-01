package org.gozaltech.nvdaremotecompanion.android;

import android.os.Bundle;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

public abstract class BaseActivity extends AppCompatActivity {

    @Override
    protected void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);
        fixActionBarTraversalOrder();
    }

    private void fixActionBarTraversalOrder() {
        View actionBarView = getWindow().getDecorView()
                .findViewById(androidx.appcompat.R.id.action_bar);
        if (actionBarView == null) return;
        View contentView = findViewById(android.R.id.content);
        if (contentView == null) return;
        contentView.setAccessibilityTraversalAfter(actionBarView.getId());
    }
}
