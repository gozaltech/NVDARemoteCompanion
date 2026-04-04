package org.gozaltech.nvdaremotecompanion.android;

import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.ViewCompat;

class AboutTab {

    private final AppCompatActivity activity;

    AboutTab(AppCompatActivity activity) {
        this.activity = activity;
    }

    View buildView() {
        LinearLayout content = new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(24, 16, 24, 16);

        TextView appNameView = new TextView(activity);
        appNameView.setText(R.string.app_name);
        appNameView.setTextSize(22f);
        ViewCompat.setAccessibilityHeading(appNameView, true);
        content.addView(appNameView);

        TextView versionView = new TextView(activity);
        versionView.setText(activity.getString(R.string.about_version, resolveVersionName()));
        versionView.setPadding(0, 4, 0, 16);
        content.addView(versionView);

        TextView descView = new TextView(activity);
        descView.setText(R.string.about_description);
        descView.setPadding(0, 0, 0, 16);
        content.addView(descView);

        TextView authorView = new TextView(activity);
        authorView.setText(R.string.about_author);
        authorView.setPadding(0, 0, 0, 16);
        content.addView(authorView);

        Button githubBtn = new Button(activity);
        githubBtn.setText(R.string.about_github);
        githubBtn.setOnClickListener(v -> openUrl("https://github.com/gozaltech/NVDARemoteCompanion"));
        content.addView(githubBtn);

        Button telegramBtn = new Button(activity);
        telegramBtn.setText(R.string.about_contact);
        telegramBtn.setOnClickListener(v -> openUrl("https://t.me/gozaltech"));
        content.addView(telegramBtn);

        Button issueBtn = new Button(activity);
        issueBtn.setText(R.string.about_report_issue);
        issueBtn.setOnClickListener(v -> openUrl("https://github.com/gozaltech/NVDARemoteCompanion/issues"));
        content.addView(issueBtn);

        Button donateBtn = new Button(activity);
        donateBtn.setText(R.string.about_donate);
        donateBtn.setOnClickListener(v -> openUrl("https://paypal.me/gozaltech"));
        content.addView(donateBtn);

        Button checkUpdateBtn = new Button(activity);
        checkUpdateBtn.setText(R.string.update_check);
        checkUpdateBtn.setOnClickListener(v -> UpdateDialog.checkAndShow(activity));
        content.addView(checkUpdateBtn);

        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        scroll.addView(content);
        return scroll;
    }

    private String resolveVersionName() {
        try {
            PackageInfo pi = activity.getPackageManager().getPackageInfo(activity.getPackageName(), 0);
            return pi.versionName;
        } catch (PackageManager.NameNotFoundException ignored) {
            return "?";
        }
    }

    private void openUrl(String url) {
        activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
    }
}
