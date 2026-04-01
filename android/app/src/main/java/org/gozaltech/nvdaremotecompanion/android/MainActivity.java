package org.gozaltech.nvdaremotecompanion.android;

import android.Manifest;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.provider.Settings;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import androidx.lifecycle.ViewModelProvider;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends BaseActivity {

    private MainViewModel viewModel;

    private boolean serviceBound = false;

    private TextView statusView;
    private LinearLayout profilesContainer;
    private Button profilesTab;
    private Button settingsTab;
    private View profilesPanel;
    private View settingsPanel;

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
    protected void onResume() {
        super.onResume();
        viewModel.onServiceConnected(viewModel.getProfiles().getValue() != null
                ? null : null);
    }

    @Override
    protected void onDestroy() {
        if (serviceBound) {
            unbindService(serviceConnection);
            serviceBound = false;
        }
        super.onDestroy();
    }

    private void observeViewModel() {
        viewModel.getProfiles().observe(this, this::renderProfiles);
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

        profilesPanel = buildProfilesPanel();
        content.addView(profilesPanel);

        settingsPanel = buildSettingsPanel();
        settingsPanel.setVisibility(View.GONE);
        content.addView(settingsPanel);

        root.addView(content);

        View divider = new View(this);
        divider.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 2));
        divider.setBackgroundColor(0xFFCCCCCC);
        root.addView(divider);

        root.addView(buildTabBar());
        setContentView(root);
        showTab(true);
    }

    private View buildProfilesPanel() {
        LinearLayout profilesContent = new LinearLayout(this);
        profilesContent.setOrientation(LinearLayout.VERTICAL);
        profilesContent.setPadding(24, 16, 24, 16);

        Button addBtn = new Button(this);
        addBtn.setText(R.string.add_profile);
        addBtn.setContentDescription(getString(R.string.add_profile));
        addBtn.setOnClickListener(v ->
                startActivity(new Intent(this, ProfileEditActivity.class)));
        profilesContent.addView(addBtn);

        profilesContainer = new LinearLayout(this);
        profilesContainer.setOrientation(LinearLayout.VERTICAL);
        profilesContainer.setContentDescription(getString(R.string.profiles_list));
        profilesContent.addView(profilesContainer);

        ScrollView scroll = new ScrollView(this);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        scroll.addView(profilesContent);
        return scroll;
    }

    private View buildSettingsPanel() {
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(24, 16, 24, 16);

        Button accessibilityBtn = new Button(this);
        accessibilityBtn.setText(R.string.enable_accessibility_service);
        accessibilityBtn.setOnClickListener(v -> openAccessibilitySettings());
        content.addView(accessibilityBtn);

        CheckBox autoConnectCb = new CheckBox(this);
        autoConnectCb.setText(R.string.pref_auto_connect);
        autoConnectCb.setChecked(AppPrefs.getAutoConnect(this));
        autoConnectCb.setOnCheckedChangeListener((cb, checked) ->
                AppPrefs.setAutoConnect(this, checked));
        content.addView(autoConnectCb);

        CheckBox autostartCb = new CheckBox(this);
        autostartCb.setText(R.string.pref_autostart);
        autostartCb.setChecked(AppPrefs.getAutostart(this));
        autostartCb.setOnCheckedChangeListener((cb, checked) ->
                AppPrefs.setAutostart(this, checked));
        content.addView(autostartCb);

        Button ttsBtn = new Button(this);
        ttsBtn.setText(R.string.tts_engine);
        ttsBtn.setContentDescription(getString(R.string.select_tts_engine));
        ttsBtn.setOnClickListener(v -> showTtsEnginePicker());
        content.addView(ttsBtn);

        CheckBox streamCb = new CheckBox(this);
        streamCb.setText(R.string.pref_accessibility_stream);
        streamCb.setChecked(AppPrefs.getAccessibilityStream(this));
        streamCb.setOnCheckedChangeListener((cb, checked) ->
                viewModel.setUseAccessibilityStream(checked));
        content.addView(streamCb);

        Button exportBtn = new Button(this);
        exportBtn.setText(R.string.export_config);
        exportBtn.setOnClickListener(v -> exportLauncher.launch("nvdaremote_config.json"));
        content.addView(exportBtn);

        Button importBtn = new Button(this);
        importBtn.setText(R.string.import_config);
        importBtn.setOnClickListener(v ->
                importLauncher.launch(new String[]{"application/json", "application/octet-stream", "*/*"}));
        content.addView(importBtn);

        ScrollView scroll = new ScrollView(this);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        scroll.addView(content);
        return scroll;
    }

    private View buildTabBar() {
        LinearLayout tabBar = new LinearLayout(this);
        tabBar.setOrientation(LinearLayout.HORIZONTAL);
        ViewCompat.setAccessibilityDelegate(tabBar, new AccessibilityDelegateCompat() {
            @Override
            public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info) {
                super.onInitializeAccessibilityNodeInfo(host, info);
                info.setCollectionInfo(
                        AccessibilityNodeInfoCompat.CollectionInfoCompat.obtain(1, 2, false));
            }
        });

        LinearLayout.LayoutParams tabLp = new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);

        profilesTab = new Button(this);
        profilesTab.setText(R.string.tab_profiles);
        profilesTab.setLayoutParams(tabLp);
        profilesTab.setOnClickListener(v -> showTab(true));

        settingsTab = new Button(this);
        settingsTab.setText(R.string.tab_settings);
        settingsTab.setLayoutParams(tabLp);
        settingsTab.setOnClickListener(v -> showTab(false));

        tabBar.addView(profilesTab);
        tabBar.addView(settingsTab);
        return tabBar;
    }

    private void showTab(boolean profiles) {
        profilesPanel.setVisibility(profiles ? View.VISIBLE : View.GONE);
        settingsPanel.setVisibility(profiles ? View.GONE : View.VISIBLE);
        applyTabDelegate(profilesTab, 0, profiles);
        applyTabDelegate(settingsTab, 1, !profiles);
        profilesTab.setSelected(profiles);
        settingsTab.setSelected(!profiles);
        profilesTab.setBackgroundColor(profiles  ? 0xFFBBDDFF : 0x00000000);
        settingsTab.setBackgroundColor(!profiles ? 0xFFBBDDFF : 0x00000000);
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

    private void renderProfiles(List<ProfileUiState> profiles) {
        profilesContainer.removeAllViews();
        if (profiles == null || profiles.isEmpty()) {
            TextView empty = new TextView(this);
            empty.setText(R.string.no_profiles);
            profilesContainer.addView(empty);
            return;
        }
        for (ProfileUiState p : profiles) {
            profilesContainer.addView(buildProfileCard(p));
        }
    }

    private View buildProfileCard(ProfileUiState p) {
        String statusText = p.active  ? getString(R.string.profile_active)
                : p.connected         ? getString(R.string.profile_connected)
                :                       getString(R.string.profile_disconnected);

        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(16, 16, 16, 16);
        card.setBackgroundColor(p.active ? 0xFFDDEEFF : 0xFFF5F5F5);
        LinearLayout.LayoutParams cardLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        cardLp.setMargins(0, 8, 0, 8);
        card.setLayoutParams(cardLp);
        card.setFocusable(true);
        card.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
        card.setContentDescription(p.displayName + " — " + statusText);

        TextView nameView = new TextView(this);
        nameView.setText(p.displayName + " — " + statusText);
        nameView.setTextSize(16f);
        nameView.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        card.addView(nameView);

        LinearLayout buttonRow = new LinearLayout(this);
        buttonRow.setOrientation(LinearLayout.HORIZONTAL);
        buttonRow.setImportantForAccessibility(
                View.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS);

        Button connectBtn = new Button(this);
        connectBtn.setText(p.connected ? R.string.disconnect : R.string.connect);
        connectBtn.setOnClickListener(v -> {
            if (p.connected) viewModel.disconnect(p.index);
            else viewModel.connect(p.index, p.displayName);
        });
        buttonRow.addView(connectBtn);

        if (p.connected) {
            Button activeBtn = new Button(this);
            activeBtn.setText(p.active ? R.string.go_local : R.string.set_active);
            activeBtn.setOnClickListener(v -> {
                if (p.active) viewModel.goLocal();
                else viewModel.setActiveProfile(p.index);
            });
            buttonRow.addView(activeBtn);
        }

        Button editBtn = new Button(this);
        editBtn.setText(R.string.edit);
        editBtn.setOnClickListener(v -> startActivity(
                new Intent(this, ProfileEditActivity.class)
                        .putExtra(ProfileEditActivity.EXTRA_PROFILE_INDEX, p.index)));
        buttonRow.addView(editBtn);
        card.addView(buttonRow);

        if (p.connected) {
            ViewCompat.addAccessibilityAction(card,
                    getString(R.string.disconnect_profile, p.displayName),
                    (v, a) -> { viewModel.disconnect(p.index); return true; });
            if (p.active) {
                ViewCompat.addAccessibilityAction(card, getString(R.string.go_local),
                        (v, a) -> { viewModel.goLocal(); return true; });
            } else {
                ViewCompat.addAccessibilityAction(card,
                        getString(R.string.set_active_profile, p.displayName),
                        (v, a) -> { viewModel.setActiveProfile(p.index); return true; });
            }
        } else {
            ViewCompat.addAccessibilityAction(card,
                    getString(R.string.connect_profile, p.displayName),
                    (v, a) -> { viewModel.connect(p.index, p.displayName); return true; });
        }
        ViewCompat.addAccessibilityAction(card,
                getString(R.string.edit_profile, p.displayName),
                (v, a) -> { startActivity(new Intent(this, ProfileEditActivity.class)
                        .putExtra(ProfileEditActivity.EXTRA_PROFILE_INDEX, p.index));
                    return true; });
        ViewCompat.addAccessibilityAction(card,
                getString(R.string.delete_profile),
                (v, a) -> { confirmDelete(p.index, p.displayName); return true; });

        return card;
    }

    private void confirmDelete(int index, String name) {
        new AlertDialog.Builder(this)
                .setTitle(R.string.delete_profile)
                .setMessage(getString(R.string.delete_confirm_message, name))
                .setPositiveButton(android.R.string.ok,
                        (d, w) -> viewModel.deleteProfile(index))
                .setNegativeButton(android.R.string.cancel, null)
                .show();
    }

    private void openAccessibilitySettings() {
        new AlertDialog.Builder(this)
                .setTitle(R.string.accessibility_dialog_title)
                .setMessage(R.string.accessibility_dialog_message)
                .setPositiveButton(R.string.open_settings, (d, w) ->
                        startActivity(new Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS)))
                .setNegativeButton(android.R.string.cancel, null)
                .show();
    }

    private void showTtsEnginePicker() {
        List<TtsManager.TtsEngineInfo> engines = TtsManager.getAvailableEngines(this);
        String currentPkg = AppPrefs.getTtsEngine(this);

        List<String> labels = new ArrayList<>();
        List<String> packages = new ArrayList<>();
        labels.add(getString(R.string.tts_engine_default));
        packages.add(null);
        for (TtsManager.TtsEngineInfo e : engines) {
            labels.add(e.label);
            packages.add(e.packageName);
        }

        int checked = Math.max(0, packages.indexOf(currentPkg));
        new AlertDialog.Builder(this)
                .setTitle(R.string.select_tts_engine)
                .setSingleChoiceItems(labels.toArray(new String[0]), checked, (d, which) -> {
                    viewModel.setTtsEngine(packages.get(which));
                    d.dismiss();
                })
                .setNegativeButton(android.R.string.cancel, null)
                .show();
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
