package org.gozaltech.nvdaremotecompanion.android;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.ViewCompat;

import java.util.List;

class ProfilesTab {

    private final AppCompatActivity activity;
    private final MainViewModel viewModel;
    private LinearLayout profilesContainer;

    ProfilesTab(AppCompatActivity activity, MainViewModel viewModel) {
        this.activity = activity;
        this.viewModel = viewModel;
    }

    View buildView() {
        LinearLayout content = new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(24, 16, 24, 16);

        Button addBtn = new Button(activity);
        addBtn.setText(R.string.add_profile);
        addBtn.setContentDescription(activity.getString(R.string.add_profile));
        addBtn.setOnClickListener(v ->
                activity.startActivity(new Intent(activity, ProfileEditActivity.class)));
        content.addView(addBtn);

        Switch sendKeysSwitch = new Switch(activity);
        sendKeysSwitch.setText(R.string.send_keys);
        sendKeysSwitch.setOnCheckedChangeListener((btn, checked) -> {
            if (checked != NativeBridge.nativeIsSendingKeys()) viewModel.toggleForwarding();
        });
        viewModel.getSendingKeys().observe(activity, sendKeysSwitch::setChecked);
        content.addView(sendKeysSwitch);

        Button clipBtn = new Button(activity);
        clipBtn.setText(R.string.send_clipboard);
        clipBtn.setContentDescription(activity.getString(R.string.send_clipboard));
        clipBtn.setOnClickListener(v -> sendClipboard());
        content.addView(clipBtn);

        profilesContainer = new LinearLayout(activity);
        profilesContainer.setOrientation(LinearLayout.VERTICAL);
        profilesContainer.setContentDescription(activity.getString(R.string.profiles_list));
        content.addView(profilesContainer);

        viewModel.getProfiles().observe(activity, this::renderProfiles);

        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        scroll.addView(content);
        return scroll;
    }

    private void renderProfiles(List<ProfileUiState> profiles) {
        profilesContainer.removeAllViews();
        if (profiles == null || profiles.isEmpty()) {
            TextView empty = new TextView(activity);
            empty.setText(R.string.no_profiles);
            profilesContainer.addView(empty);
            return;
        }
        for (ProfileUiState p : profiles) {
            profilesContainer.addView(buildProfileCard(p));
        }
    }

    private View buildProfileCard(ProfileUiState p) {
        String statusText = p.active   ? activity.getString(R.string.profile_active)
                : p.connected          ? activity.getString(R.string.profile_connected)
                :                        activity.getString(R.string.profile_disconnected);

        LinearLayout card = new LinearLayout(activity);
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

        TextView nameView = new TextView(activity);
        nameView.setText(p.displayName + " — " + statusText);
        nameView.setTextSize(16f);
        nameView.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        card.addView(nameView);

        LinearLayout buttonRow = new LinearLayout(activity);
        buttonRow.setOrientation(LinearLayout.HORIZONTAL);
        buttonRow.setImportantForAccessibility(
                View.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS);

        Button connectBtn = new Button(activity);
        connectBtn.setText(p.connected ? R.string.disconnect : R.string.connect);
        connectBtn.setOnClickListener(v -> {
            if (p.connected) viewModel.disconnect(p.index);
            else viewModel.connect(p.index, p.displayName);
        });
        buttonRow.addView(connectBtn);

        if (p.connected && !p.active) {
            Button activeBtn = new Button(activity);
            activeBtn.setText(R.string.set_active);
            activeBtn.setOnClickListener(v -> viewModel.setActiveProfile(p.index));
            buttonRow.addView(activeBtn);
        }

        Button editBtn = new Button(activity);
        editBtn.setText(R.string.edit);
        editBtn.setOnClickListener(v -> activity.startActivity(
                new Intent(activity, ProfileEditActivity.class)
                        .putExtra(ProfileEditActivity.EXTRA_PROFILE_INDEX, p.index)));
        buttonRow.addView(editBtn);
        card.addView(buttonRow);

        if (p.connected) {
            ViewCompat.addAccessibilityAction(card,
                    activity.getString(R.string.disconnect_profile, p.displayName),
                    (v, a) -> { viewModel.disconnect(p.index); return true; });
            if (!p.active) {
                ViewCompat.addAccessibilityAction(card,
                        activity.getString(R.string.set_active_profile, p.displayName),
                        (v, a) -> { viewModel.setActiveProfile(p.index); return true; });
            }
            ViewCompat.addAccessibilityAction(card,
                    activity.getString(R.string.send_clipboard),
                    (v, a) -> { sendClipboardToProfile(p.index); return true; });
        } else {
            ViewCompat.addAccessibilityAction(card,
                    activity.getString(R.string.connect_profile, p.displayName),
                    (v, a) -> { viewModel.connect(p.index, p.displayName); return true; });
        }
        ViewCompat.addAccessibilityAction(card,
                activity.getString(R.string.edit_profile, p.displayName),
                (v, a) -> {
                    activity.startActivity(new Intent(activity, ProfileEditActivity.class)
                            .putExtra(ProfileEditActivity.EXTRA_PROFILE_INDEX, p.index));
                    return true;
                });
        ViewCompat.addAccessibilityAction(card,
                activity.getString(R.string.delete_profile),
                (v, a) -> { confirmDelete(p.index, p.displayName); return true; });

        return card;
    }

    private void sendClipboard() {
        int activeProfile = NativeBridge.nativeGetActiveProfile();
        if (activeProfile < 0) {
            Toast.makeText(activity, R.string.clipboard_not_connected, Toast.LENGTH_SHORT).show();
            return;
        }
        sendClipboardToProfile(activeProfile);
    }

    private void sendClipboardToProfile(int profileIndex) {
        ClipboardManager clipboard = (ClipboardManager)
                activity.getSystemService(Context.CLIPBOARD_SERVICE);
        if (clipboard == null || !clipboard.hasPrimaryClip()) {
            Toast.makeText(activity, R.string.clipboard_empty, Toast.LENGTH_SHORT).show();
            return;
        }
        ClipData.Item item = clipboard.getPrimaryClip().getItemAt(0);
        CharSequence text = item != null ? item.getText() : null;
        if (text == null || text.length() == 0) {
            Toast.makeText(activity, R.string.clipboard_empty, Toast.LENGTH_SHORT).show();
            return;
        }
        NativeBridge.nativeSendClipboardText(text.toString(), profileIndex);
        Toast.makeText(activity, R.string.clipboard_sent, Toast.LENGTH_SHORT).show();
    }

    private void confirmDelete(int index, String name) {
        new AlertDialog.Builder(activity)
                .setTitle(R.string.delete_profile)
                .setMessage(activity.getString(R.string.delete_confirm_message, name))
                .setPositiveButton(android.R.string.ok,
                        (d, w) -> viewModel.deleteProfile(index))
                .setNegativeButton(android.R.string.cancel, null)
                .show();
    }
}
