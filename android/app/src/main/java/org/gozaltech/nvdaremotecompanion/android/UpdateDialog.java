package org.gozaltech.nvdaremotecompanion.android;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import java.io.File;

class UpdateDialog {

    static void checkAndShowIfNeeded(AppCompatActivity activity) {
        String current = resolveVersionName(activity);
        UpdateChecker.checkAsync(current, new UpdateChecker.Callback() {
            @Override
            public void onResult(UpdateInfo update) {
                if (update != null) {
                    activity.runOnUiThread(() -> show(activity, update, current));
                }
            }

            @Override
            public void onError(Exception e) {
            }
        });
    }

    static void checkAndShow(AppCompatActivity activity) {
        String current = resolveVersionName(activity);
        AlertDialog checking = new AlertDialog.Builder(activity)
                .setTitle(R.string.update_checking)
                .setMessage("…")
                .setCancelable(false)
                .create();
        checking.show();

        UpdateChecker.checkAsync(current, new UpdateChecker.Callback() {
            @Override
            public void onResult(UpdateInfo update) {
                activity.runOnUiThread(() -> {
                    checking.dismiss();
                    if (update != null) {
                        show(activity, update, current);
                    } else {
                        new AlertDialog.Builder(activity)
                                .setTitle(R.string.update_check)
                                .setMessage(R.string.update_up_to_date)
                                .setPositiveButton(android.R.string.ok, null)
                                .show();
                    }
                });
            }

            @Override
            public void onError(Exception e) {
                activity.runOnUiThread(() -> {
                    checking.dismiss();
                    new AlertDialog.Builder(activity)
                            .setTitle(R.string.update_check)
                            .setMessage(R.string.update_error)
                            .setPositiveButton(android.R.string.ok, null)
                            .show();
                });
            }
        });
    }

    private static void show(AppCompatActivity activity, UpdateInfo update, String currentVersion) {
        if (activity.isFinishing() || activity.isDestroyed()) return;

        String notes = update.releaseNotes != null && !update.releaseNotes.isEmpty()
                ? update.releaseNotes
                : activity.getString(R.string.update_no_changelog);

        new AlertDialog.Builder(activity)
                .setTitle(R.string.update_available_title)
                .setView(buildInfoView(activity, currentVersion, update.version, notes))
                .setPositiveButton(R.string.update_download,
                        (d, w) -> showDownloadProgress(activity, update))
                .setNegativeButton(R.string.update_later, null)
                .show();
    }

    private static View buildInfoView(AppCompatActivity activity,
                                      String currentVersion, String newVersion, String notes) {
        LinearLayout layout = new LinearLayout(activity);
        layout.setOrientation(LinearLayout.VERTICAL);
        int pad = dpToPx(activity, 16);
        layout.setPadding(pad, pad / 2, pad, 0);

        TextView versions = new TextView(activity);
        versions.setText(activity.getString(R.string.update_version_line,
                currentVersion, newVersion));
        versions.setPadding(0, 0, 0, dpToPx(activity, 12));
        layout.addView(versions);

        TextView notesLabel = new TextView(activity);
        notesLabel.setText(R.string.update_changelog);
        notesLabel.setTextSize(14f);
        layout.addView(notesLabel);

        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dpToPx(activity, 200)));

        LinearLayout notesList = new LinearLayout(activity);
        notesList.setOrientation(LinearLayout.VERTICAL);
        notesList.setPadding(0, dpToPx(activity, 4), 0, 0);

        String[] lines = notes.split("\n");
        for (String line : lines) {
            String trimmed = line.trim();
            if (trimmed.isEmpty()) continue;
            TextView item = new TextView(activity);
            item.setText(trimmed);
            item.setPadding(0, dpToPx(activity, 2), 0, dpToPx(activity, 2));
            notesList.addView(item);
        }

        scroll.addView(notesList);
        layout.addView(scroll);

        return layout;
    }

    private static void showDownloadProgress(AppCompatActivity activity, UpdateInfo update) {
        if (activity.isFinishing() || activity.isDestroyed()) return;

        LinearLayout layout = new LinearLayout(activity);
        layout.setOrientation(LinearLayout.VERTICAL);
        int pad = dpToPx(activity, 16);
        layout.setPadding(pad, pad, pad, pad / 2);

        ProgressBar progressBar = new ProgressBar(activity, null,
                android.R.attr.progressBarStyleHorizontal);
        progressBar.setMax(100);
        progressBar.setProgress(0);
        layout.addView(progressBar);

        TextView progressText = new TextView(activity);
        progressText.setText(activity.getString(R.string.update_downloading, 0));
        progressText.setPadding(0, dpToPx(activity, 8), 0, 0);
        layout.addView(progressText);

        AlertDialog[] dialogRef = new AlertDialog[1];
        boolean[] cancelled = {false};

        AlertDialog dialog = new AlertDialog.Builder(activity)
                .setTitle(R.string.update_downloading_title)
                .setView(layout)
                .setCancelable(false)
                .setNegativeButton(android.R.string.cancel, null)
                .create();

        dialogRef[0] = dialog;
        dialog.show();

        dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener(v ->
                new AlertDialog.Builder(activity)
                        .setMessage(R.string.update_cancel_confirm)
                        .setPositiveButton(android.R.string.yes, (d, w) -> {
                            cancelled[0] = true;
                            dialog.dismiss();
                        })
                        .setNegativeButton(android.R.string.no, null)
                        .show());

        UpdateInstaller.downloadAsync(activity, update.downloadUrl, new UpdateInstaller.ProgressCallback() {
            @Override
            public void onProgress(int percent) {
                activity.runOnUiThread(() -> {
                    if (cancelled[0]) return;
                    progressBar.setProgress(percent);
                    progressText.setText(activity.getString(R.string.update_downloading, percent));
                });
            }

            @Override
            public void onDone(File apk) {
                if (cancelled[0]) { apk.delete(); return; }

                if (!UpdateInstaller.hasSameSignature(activity, apk)) {
                    apk.delete();
                    activity.runOnUiThread(() -> {
                        dialog.dismiss();
                        new AlertDialog.Builder(activity)
                                .setTitle(R.string.update_check)
                                .setMessage(R.string.update_signature_mismatch)
                                .setPositiveButton(android.R.string.ok, null)
                                .show();
                    });
                    return;
                }

                activity.runOnUiThread(() -> {
                    if (activity.isFinishing() || activity.isDestroyed()) return;
                    dialog.dismiss();
                    new AlertDialog.Builder(activity)
                            .setTitle(R.string.update_available_title)
                            .setMessage(activity.getString(R.string.update_ready, update.version))
                            .setPositiveButton(R.string.update_install,
                                    (d, w) -> UpdateInstaller.install(activity, apk))
                            .setNegativeButton(R.string.update_later, null)
                            .show();
                });
            }

            @Override
            public void onError(Exception e) {
                if (cancelled[0]) return;
                activity.runOnUiThread(() -> {
                    dialog.dismiss();
                    new AlertDialog.Builder(activity)
                            .setTitle(R.string.update_check)
                            .setMessage(R.string.update_error)
                            .setPositiveButton(android.R.string.ok, null)
                            .show();
                });
            }
        });
    }

    private static String resolveVersionName(AppCompatActivity activity) {
        try {
            PackageInfo pi = activity.getPackageManager()
                    .getPackageInfo(activity.getPackageName(), 0);
            return pi.versionName;
        } catch (PackageManager.NameNotFoundException ignored) {
            return "0";
        }
    }

    private static int dpToPx(AppCompatActivity activity, int dp) {
        float density = activity.getResources().getDisplayMetrics().density;
        return Math.round(dp * density);
    }
}
