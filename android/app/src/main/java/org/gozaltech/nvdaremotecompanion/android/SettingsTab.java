package org.gozaltech.nvdaremotecompanion.android;

import android.app.LocaleConfig;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.LocaleList;
import android.os.PowerManager;
import android.provider.Settings;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.os.LocaleListCompat;
import androidx.core.view.ViewCompat;

import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;

class SettingsTab {

    private interface SliderCallback {
        void onValue(float value);
    }

    private final AppCompatActivity activity;
    private final MainViewModel viewModel;
    private final ActivityResultLauncher<String> exportLauncher;
    private final ActivityResultLauncher<String[]> importLauncher;

    SettingsTab(AppCompatActivity activity, MainViewModel viewModel,
                ActivityResultLauncher<String> exportLauncher,
                ActivityResultLauncher<String[]> importLauncher) {
        this.activity = activity;
        this.viewModel = viewModel;
        this.exportLauncher = exportLauncher;
        this.importLauncher = importLauncher;
    }

    View buildView() {
        LinearLayout content = new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(24, 16, 24, 16);

        Button accessibilityBtn = new Button(activity);
        accessibilityBtn.setText(R.string.enable_accessibility_service);
        accessibilityBtn.setOnClickListener(v -> openAccessibilitySettings());
        content.addView(accessibilityBtn);

        Button batteryBtn = new Button(activity);
        batteryBtn.setText(R.string.battery_optimization);
        batteryBtn.setOnClickListener(v -> requestBatteryOptimizationExemption());
        content.addView(batteryBtn);

        CheckBox autoConnectCb = new CheckBox(activity);
        autoConnectCb.setText(R.string.pref_auto_connect);
        autoConnectCb.setChecked(AppPrefs.getAutoConnect(activity));
        autoConnectCb.setOnCheckedChangeListener((cb, checked) ->
                AppPrefs.setAutoConnect(activity, checked));
        content.addView(autoConnectCb);

        CheckBox autostartCb = new CheckBox(activity);
        autostartCb.setText(R.string.pref_autostart);
        autostartCb.setChecked(AppPrefs.getAutostart(activity));
        autostartCb.setOnCheckedChangeListener((cb, checked) ->
                AppPrefs.setAutostart(activity, checked));
        content.addView(autostartCb);

        content.addView(buildSpeechOutputSection());

        Button languageBtn = new Button(activity);
        languageBtn.setText(activity.getString(R.string.pref_language) + ": " + currentLanguageLabel());
        languageBtn.setOnClickListener(v -> showLanguagePicker());
        content.addView(languageBtn);

        Button exportBtn = new Button(activity);
        exportBtn.setText(R.string.export_config);
        exportBtn.setOnClickListener(v -> exportLauncher.launch("nvdaremote_config.json"));
        content.addView(exportBtn);

        Button importBtn = new Button(activity);
        importBtn.setText(R.string.import_config);
        importBtn.setOnClickListener(v ->
                importLauncher.launch(new String[]{"application/json", "application/octet-stream", "*/*"}));
        content.addView(importBtn);

        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        scroll.addView(content);
        return scroll;
    }

    private View buildSpeechOutputSection() {
        LinearLayout section = new LinearLayout(activity);
        section.setOrientation(LinearLayout.VERTICAL);

        TextView header = new TextView(activity);
        header.setText(R.string.speech_output_header);
        header.setTextSize(16f);
        header.setPadding(0, 16, 0, 4);
        ViewCompat.setAccessibilityHeading(header, true);
        section.addView(header);

        int currentMode = AppPrefs.getSpeechOutputMode(activity);

        RadioGroup modeGroup = new RadioGroup(activity);
        modeGroup.setOrientation(RadioGroup.VERTICAL);

        RadioButton rbTts = new RadioButton(activity);
        rbTts.setText(R.string.speech_mode_tts);
        rbTts.setId(View.generateViewId());

        RadioButton rbSr = new RadioButton(activity);
        rbSr.setText(R.string.speech_mode_screen_reader);
        rbSr.setId(View.generateViewId());

        RadioButton rbDirect = new RadioButton(activity);
        rbDirect.setText(R.string.speech_mode_direct_tts);
        rbDirect.setId(View.generateViewId());

        modeGroup.addView(rbTts);
        modeGroup.addView(rbSr);
        modeGroup.addView(rbDirect);

        int checkedId = currentMode == 1 ? rbSr.getId()
                      : currentMode == 2 ? rbDirect.getId()
                      : rbTts.getId();
        modeGroup.check(checkedId);
        section.addView(modeGroup);

        LinearLayout ttsOptions = new LinearLayout(activity);
        ttsOptions.setOrientation(LinearLayout.VERTICAL);
        ttsOptions.setVisibility(currentMode == 0 ? View.VISIBLE : View.GONE);

        Button ttsEngineBtn = new Button(activity);
        ttsEngineBtn.setText(R.string.tts_engine);
        ttsEngineBtn.setContentDescription(activity.getString(R.string.select_tts_engine));
        ttsEngineBtn.setOnClickListener(v -> showTtsEnginePicker());
        ttsOptions.addView(ttsEngineBtn);

        CheckBox streamCb = new CheckBox(activity);
        streamCb.setText(R.string.pref_accessibility_stream);
        streamCb.setChecked(AppPrefs.getAccessibilityStream(activity));
        streamCb.setOnCheckedChangeListener((cb, checked) ->
                viewModel.setUseAccessibilityStream(checked));
        ttsOptions.addView(streamCb);

        ttsOptions.addView(buildSlider(
                activity.getString(R.string.tts_pitch),
                AppPrefs.getTtsPitch(activity), 0.5f, 2.0f,
                viewModel::setTtsPitch));
        ttsOptions.addView(buildSlider(
                activity.getString(R.string.tts_rate),
                AppPrefs.getTtsRate(activity), 0.5f, 2.0f,
                viewModel::setTtsRate));
        ttsOptions.addView(buildSlider(
                activity.getString(R.string.tts_volume),
                AppPrefs.getTtsVolume(activity), 0.0f, 1.0f,
                viewModel::setTtsVolume));

        section.addView(ttsOptions);

        LinearLayout directOptions = new LinearLayout(activity);
        directOptions.setOrientation(LinearLayout.VERTICAL);
        directOptions.setVisibility(currentMode == 2 ? View.VISIBLE : View.GONE);
        directOptions.addView(buildDirectEngineSpinner());
        section.addView(directOptions);

        modeGroup.setOnCheckedChangeListener((group, id) -> {
            int mode = id == rbSr.getId() ? 1 : id == rbDirect.getId() ? 2 : 0;
            viewModel.setSpeechOutputMode(mode);
            ttsOptions.setVisibility(mode == 0 ? View.VISIBLE : View.GONE);
            directOptions.setVisibility(mode == 2 ? View.VISIBLE : View.GONE);
        });

        return section;
    }

    private View buildDirectEngineSpinner() {
        LinearLayout row = new LinearLayout(activity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setPadding(0, 8, 0, 0);

        TextView label = new TextView(activity);
        label.setText(R.string.direct_tts_engine_label);
        label.setPadding(0, 0, 16, 0);
        row.addView(label);

        String[] engines = { "ru_tts" };
        String current = AppPrefs.getDirectTtsEngine(activity);

        Spinner spinner = new Spinner(activity);
        ArrayAdapter<String> adapter = new ArrayAdapter<>(
                activity, android.R.layout.simple_spinner_item, engines);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);

        int sel = 0;
        for (int i = 0; i < engines.length; i++) {
            if (engines[i].equals(current)) { sel = i; break; }
        }
        spinner.setSelection(sel);

        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View v, int pos, long id) {
                viewModel.setDirectTtsEngine(engines[pos]);
            }
            @Override public void onNothingSelected(AdapterView<?> parent) {}
        });

        row.addView(spinner);
        return row;
    }

    private View buildSlider(String label, float current, float min, float max,
                             SliderCallback callback) {
        LinearLayout row = new LinearLayout(activity);
        row.setOrientation(LinearLayout.VERTICAL);
        row.setPadding(0, 8, 0, 0);

        int steps = 100;
        int initialProgress = Math.round((current - min) / (max - min) * steps);

        TextView labelView = new TextView(activity);
        labelView.setText(label + ": " + String.format(Locale.ROOT, "%.1f", current));
        row.addView(labelView);

        SeekBar seekBar = new SeekBar(activity);
        seekBar.setMax(steps);
        seekBar.setProgress(initialProgress);
        seekBar.setContentDescription(label);
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                if (!fromUser) return;
                float value = min + (float) progress / steps * (max - min);
                labelView.setText(label + ": " + String.format(Locale.ROOT, "%.1f", value));
                callback.onValue(value);
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) {}
        });
        row.addView(seekBar);

        return row;
    }

    private void openAccessibilitySettings() {
        new AlertDialog.Builder(activity)
                .setTitle(R.string.accessibility_dialog_title)
                .setMessage(R.string.accessibility_dialog_message)
                .setPositiveButton(R.string.open_settings, (d, w) ->
                        activity.startActivity(new Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS)))
                .setNegativeButton(android.R.string.cancel, null)
                .show();
    }

    private void requestBatteryOptimizationExemption() {
        PowerManager pm = (PowerManager) activity.getSystemService(AppCompatActivity.POWER_SERVICE);
        if (pm.isIgnoringBatteryOptimizations(activity.getPackageName())) {
            Toast.makeText(activity, R.string.battery_optimization_already_exempt, Toast.LENGTH_SHORT).show();
            return;
        }
        activity.startActivity(new Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS,
                Uri.parse("package:" + activity.getPackageName())));
    }

    private void showTtsEnginePicker() {
        List<TtsManager.TtsEngineInfo> engines = TtsManager.getAvailableEngines(activity);
        String currentPkg = AppPrefs.getTtsEngine(activity);

        List<String> labels = new ArrayList<>();
        List<String> packages = new ArrayList<>();
        labels.add(activity.getString(R.string.tts_engine_default));
        packages.add(null);
        for (TtsManager.TtsEngineInfo e : engines) {
            labels.add(e.label);
            packages.add(e.packageName);
        }

        int checked = Math.max(0, packages.indexOf(currentPkg));
        new AlertDialog.Builder(activity)
                .setTitle(R.string.select_tts_engine)
                .setSingleChoiceItems(labels.toArray(new String[0]), checked, (d, which) -> {
                    viewModel.setTtsEngine(packages.get(which));
                    d.dismiss();
                })
                .setNegativeButton(android.R.string.cancel, null)
                .show();
    }

    private List<String> getSupportedLocaleTags() {
        List<String> tags = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            LocaleConfig config = new LocaleConfig(activity);
            LocaleList supported = config.getSupportedLocales();
            if (supported != null) {
                for (int i = 0; i < supported.size(); i++) {
                    tags.add(supported.get(i).toLanguageTag());
                }
            }
        } else {
            LinkedHashSet<String> seen = new LinkedHashSet<>();
            for (String locale : activity.getAssets().getLocales()) {
                String lang = Locale.forLanguageTag(locale).getLanguage();
                if (!lang.isEmpty()) seen.add(lang);
            }
            tags.addAll(seen);
        }
        return tags;
    }

    private String currentLanguageLabel() {
        LocaleListCompat current = AppCompatDelegate.getApplicationLocales();
        if (current.isEmpty()) return activity.getString(R.string.language_system);
        Locale locale = current.get(0);
        return locale.getDisplayName(locale);
    }

    private void showLanguagePicker() {
        List<String> tags = getSupportedLocaleTags();

        LocaleListCompat current = AppCompatDelegate.getApplicationLocales();
        String currentTag = current.isEmpty() ? "" : current.get(0).toLanguageTag();

        String[] labels = new String[tags.size() + 1];
        labels[0] = activity.getString(R.string.language_system);
        for (int i = 0; i < tags.size(); i++) {
            Locale locale = Locale.forLanguageTag(tags.get(i));
            labels[i + 1] = locale.getDisplayName(locale);
        }

        int checked = 0;
        for (int i = 0; i < tags.size(); i++) {
            if (Locale.forLanguageTag(tags.get(i)).getLanguage()
                    .equals(Locale.forLanguageTag(currentTag).getLanguage())) {
                checked = i + 1;
                break;
            }
        }

        new AlertDialog.Builder(activity)
                .setTitle(R.string.pref_language)
                .setSingleChoiceItems(labels, checked, (d, which) -> {
                    LocaleListCompat locales = which == 0
                            ? LocaleListCompat.getEmptyLocaleList()
                            : LocaleListCompat.forLanguageTags(tags.get(which - 1));
                    AppCompatDelegate.setApplicationLocales(locales);
                    d.dismiss();
                })
                .setNegativeButton(android.R.string.cancel, null)
                .show();
    }
}
