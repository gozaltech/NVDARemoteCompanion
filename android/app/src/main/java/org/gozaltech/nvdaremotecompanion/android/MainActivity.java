package org.gozaltech.nvdaremotecompanion.android;

import android.Manifest;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import androidx.lifecycle.ViewModelProvider;

public class MainActivity extends BaseActivity {

    private static final int TAB_PROFILES = 0;
    private static final int TAB_SETTINGS = 1;
    private static final int TAB_ABOUT    = 2;

    private MainViewModel viewModel;
    private boolean serviceBound = false;

    private TextView statusView;
    private Button profilesTab;
    private Button settingsTab;
    private Button aboutTab;
    private View profilesPanel;
    private View settingsPanel;
    private View aboutPanel;

    private final ActivityResultLauncher<String> exportLauncher =
            registerForActivityResult(new ActivityResultContracts.CreateDocument("application/json"),
                    uri -> { if (uri != null) viewModel.exportConfig(uri, getContentResolver()); });

    private final ActivityResultLauncher<String[]> importLauncher =
            registerForActivityResult(new ActivityResultContracts.OpenDocument(),
                    uri -> { if (uri != null) viewModel.importConfig(uri, getContentResolver()); });

    private final ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            serviceBound = true;
            viewModel.onServiceConnected(((ConnectionService.LocalBinder) binder).getService());
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            serviceBound = false;
            viewModel.onServiceDisconnected();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        viewModel = new ViewModelProvider(this).get(MainViewModel.class);

        buildUi();
        observeViewModel();
        requestNotificationPermission();

        Intent intent = new Intent(this, ConnectionService.class);
        startService(intent);
        bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
    }

    @Override
    protected void onDestroy() {
        if (serviceBound) {
            unbindService(serviceConnection);
            serviceBound = false;
        }
        super.onDestroy();
    }

    private void buildUi() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);

        Toolbar toolbar = new Toolbar(this);
        toolbar.setTitle(R.string.app_name);
        toolbar.setSubtitle(R.string.status_local);
        root.addView(toolbar);
        setSupportActionBar(toolbar);

        statusView = new TextView(this);
        statusView.setVisibility(View.GONE);
        root.addView(statusView);

        FrameLayout content = new FrameLayout(this);
        content.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f));

        profilesPanel = new ProfilesTab(this, viewModel).buildView();
        content.addView(profilesPanel);

        settingsPanel = new SettingsTab(this, viewModel, exportLauncher, importLauncher).buildView();
        settingsPanel.setVisibility(View.GONE);
        content.addView(settingsPanel);

        aboutPanel = new AboutTab(this).buildView();
        aboutPanel.setVisibility(View.GONE);
        content.addView(aboutPanel);

        root.addView(content);

        View divider = new View(this);
        divider.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 2));
        divider.setBackgroundColor(0xFFCCCCCC);
        root.addView(divider);

        root.addView(buildTabBar());
        setContentView(root);
        showTab(TAB_PROFILES);
    }

    private void observeViewModel() {
        viewModel.getStatusSubtitle().observe(this, subtitle -> {
            if (getSupportActionBar() != null) {
                CharSequence prev = getSupportActionBar().getSubtitle();
                if (!subtitle.equals(prev != null ? prev.toString() : "")) {
                    getSupportActionBar().setSubtitle(subtitle);
                    announceForTalkBack(subtitle);
                }
            }
        });
        viewModel.getToast().observe(this, event -> {
            Integer resId = event.consume();
            if (resId != null) Toast.makeText(this, resId, Toast.LENGTH_SHORT).show();
        });
    }

    private View buildTabBar() {
        LinearLayout tabBar = new LinearLayout(this);
        tabBar.setOrientation(LinearLayout.HORIZONTAL);
        ViewCompat.setAccessibilityDelegate(tabBar, new AccessibilityDelegateCompat() {
            @Override
            public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info) {
                super.onInitializeAccessibilityNodeInfo(host, info);
                info.setCollectionInfo(
                        AccessibilityNodeInfoCompat.CollectionInfoCompat.obtain(1, 3, false));
            }
        });

        LinearLayout.LayoutParams tabLp = new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);

        profilesTab = new Button(this);
        profilesTab.setText(R.string.tab_profiles);
        profilesTab.setLayoutParams(tabLp);
        profilesTab.setOnClickListener(v -> showTab(TAB_PROFILES));

        settingsTab = new Button(this);
        settingsTab.setText(R.string.tab_settings);
        settingsTab.setLayoutParams(tabLp);
        settingsTab.setOnClickListener(v -> showTab(TAB_SETTINGS));

        aboutTab = new Button(this);
        aboutTab.setText(R.string.tab_about);
        aboutTab.setLayoutParams(tabLp);
        aboutTab.setOnClickListener(v -> showTab(TAB_ABOUT));

        tabBar.addView(profilesTab);
        tabBar.addView(settingsTab);
        tabBar.addView(aboutTab);
        return tabBar;
    }

    private void showTab(int tab) {
        profilesPanel.setVisibility(tab == TAB_PROFILES ? View.VISIBLE : View.GONE);
        settingsPanel.setVisibility(tab == TAB_SETTINGS ? View.VISIBLE : View.GONE);
        aboutPanel.setVisibility(tab == TAB_ABOUT    ? View.VISIBLE : View.GONE);
        applyTabDelegate(profilesTab, 0, tab == TAB_PROFILES);
        applyTabDelegate(settingsTab, 1, tab == TAB_SETTINGS);
        applyTabDelegate(aboutTab,    2, tab == TAB_ABOUT);
        profilesTab.setBackgroundColor(tab == TAB_PROFILES ? 0xFFBBDDFF : 0x00000000);
        settingsTab.setBackgroundColor(tab == TAB_SETTINGS ? 0xFFBBDDFF : 0x00000000);
        aboutTab.setBackgroundColor(   tab == TAB_ABOUT    ? 0xFFBBDDFF : 0x00000000);
    }

    private void applyTabDelegate(Button tab, int tabIndex, boolean selected) {
        ViewCompat.setAccessibilityDelegate(tab, new AccessibilityDelegateCompat() {
            @Override
            public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info) {
                super.onInitializeAccessibilityNodeInfo(host, info);
                info.setRoleDescription("Tab");
                info.setSelected(selected);
                info.setCollectionItemInfo(
                        AccessibilityNodeInfoCompat.CollectionItemInfoCompat.obtain(
                                0, 1, tabIndex, 1, false, selected));
            }
        });
    }

    private void announceForTalkBack(String text) {
        AccessibilityManager am = (AccessibilityManager) getSystemService(ACCESSIBILITY_SERVICE);
        if (am != null && am.isEnabled()) statusView.announceForAccessibility(text);
    }

    private void requestNotificationPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
                        != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.POST_NOTIFICATIONS}, 0);
        }
    }
}
