package org.gozaltech.nvdaremotecompanion.android;

import android.os.Bundle;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.lifecycle.ViewModelProvider;

public class ProfileEditActivity extends BaseActivity {

    public static final String EXTRA_PROFILE_INDEX = "profile_index";

    private int profileIndex = -1;

    private EditText nameEdit;
    private EditText hostEdit;
    private EditText portEdit;
    private EditText keyEdit;
    private CheckBox speechCheck;
    private CheckBox muteCheck;
    private CheckBox soundsCheck;
    private CheckBox autoConnectCheck;

    private ProfileEditViewModel viewModel;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        profileIndex = getIntent().getIntExtra(EXTRA_PROFILE_INDEX, -1);

        setTitle(profileIndex >= 0 ? getString(R.string.edit_profile_title)
                                   : getString(R.string.add_profile_title));
        buildUi();

        viewModel = new ViewModelProvider(this).get(ProfileEditViewModel.class);
        observeViewModel();

        if (profileIndex >= 0) viewModel.loadProfile(profileIndex);
    }

    private void observeViewModel() {
        viewModel.getFormData().observe(this, data -> {
            if (data == null) return;
            nameEdit.setText(data.name);
            hostEdit.setText(data.host);
            portEdit.setText(String.valueOf(data.port));
            keyEdit.setText(data.key);
            speechCheck.setChecked(data.speech);
            soundsCheck.setChecked(data.sounds);
            muteCheck.setChecked(data.mute);
            autoConnectCheck.setChecked(data.autoConnect);
        });

        viewModel.getToast().observe(this, event -> {
            if (event == null) return;
            Integer resId = event.consume();
            if (resId != null) Toast.makeText(this, resId, Toast.LENGTH_SHORT).show();
        });

        viewModel.getFinish().observe(this, event -> {
            if (event != null && !event.isConsumed()) { event.consume(); finish(); }
        });
    }

    private void buildUi() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(24, 24, 24, 24);

        nameEdit         = addLabeledField(root, getString(R.string.field_name), "");
        hostEdit         = addLabeledField(root, getString(R.string.field_host), "");
        portEdit         = addLabeledField(root, getString(R.string.field_port), "6837");
        keyEdit          = addLabeledField(root, getString(R.string.field_key), "");
        speechCheck      = addCheckBox(root, getString(R.string.field_speech), true);
        soundsCheck      = addCheckBox(root, getString(R.string.field_sounds), true);
        muteCheck        = addCheckBox(root, getString(R.string.field_mute_on_local), false);
        autoConnectCheck = addCheckBox(root, getString(R.string.field_auto_connect), true);

        Button saveButton = new Button(this);
        saveButton.setText(R.string.save);
        saveButton.setOnClickListener(v -> onSaveClicked());
        root.addView(saveButton);

        if (profileIndex >= 0) {
            Button deleteButton = new Button(this);
            deleteButton.setText(R.string.delete_profile);
            deleteButton.setContentDescription(getString(R.string.delete_profile));
            deleteButton.setOnClickListener(v -> viewModel.delete(profileIndex));
            root.addView(deleteButton);
        }

        ScrollView scroll = new ScrollView(this);
        scroll.addView(root);
        setContentView(scroll);
    }

    private void onSaveClicked() {
        viewModel.save(
                profileIndex,
                nameEdit.getText().toString().trim(),
                hostEdit.getText().toString().trim(),
                portEdit.getText().toString().trim(),
                keyEdit.getText().toString().trim(),
                speechCheck.isChecked(),
                soundsCheck.isChecked(),
                muteCheck.isChecked(),
                autoConnectCheck.isChecked());
    }

    private EditText addLabeledField(LinearLayout parent, String label, String hint) {
        TextView tv = new TextView(this);
        tv.setText(label);
        parent.addView(tv);

        EditText field = new EditText(this);
        field.setHint(hint);
        field.setContentDescription(label);
        field.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        parent.addView(field);
        return field;
    }

    private CheckBox addCheckBox(LinearLayout parent, String label, boolean checked) {
        CheckBox cb = new CheckBox(this);
        cb.setText(label);
        cb.setChecked(checked);
        parent.addView(cb);
        return cb;
    }
}
