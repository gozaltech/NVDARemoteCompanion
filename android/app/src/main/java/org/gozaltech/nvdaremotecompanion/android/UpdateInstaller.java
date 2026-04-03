package org.gozaltech.nvdaremotecompanion.android;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;

import androidx.core.content.FileProvider;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Arrays;

class UpdateInstaller {

    interface ProgressCallback {
        void onProgress(int percent);
        void onDone(File apk);
        void onError(Exception e);
    }

    static void downloadAsync(Context context, String downloadUrl, ProgressCallback callback) {
        new Thread(() -> {
            try {
                File apk = download(context, downloadUrl, callback);
                callback.onDone(apk);
            } catch (Exception e) {
                callback.onError(e);
            }
        }, "update-downloader").start();
    }

    private static File download(Context context, String downloadUrl, ProgressCallback callback)
            throws Exception {
        File dir = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
        if (dir == null) dir = context.getCacheDir();
        File out = new File(dir, "NVDARemoteCompanion-update.apk");

        HttpURLConnection conn = (HttpURLConnection) new URL(downloadUrl).openConnection();
        conn.setConnectTimeout(15_000);
        conn.setReadTimeout(30_000);
        conn.connect();

        int total = conn.getContentLength();
        try (InputStream in = conn.getInputStream();
             FileOutputStream fos = new FileOutputStream(out)) {
            byte[] buf = new byte[8192];
            int read;
            long done = 0;
            while ((read = in.read(buf)) != -1) {
                fos.write(buf, 0, read);
                done += read;
                if (total > 0) callback.onProgress((int) (done * 100 / total));
            }
        }
        return out;
    }

    static void install(Context context, File apk) {
        try {
            installViaSession(context, apk);
        } catch (Exception ignored) {
            installInteractive(context, apk);
        }
    }

    private static void installViaSession(Context context, File apk) throws Exception {
        PackageInstaller installer = context.getPackageManager().getPackageInstaller();

        PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(
                PackageInstaller.SessionParams.MODE_FULL_INSTALL);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            params.setRequireUserAction(PackageInstaller.SessionParams.USER_ACTION_NOT_REQUIRED);
        }

        int sessionId = installer.createSession(params);
        try (PackageInstaller.Session session = installer.openSession(sessionId)) {
            try (InputStream in = new FileInputStream(apk);
                 OutputStream out = session.openWrite("package", 0, apk.length())) {
                byte[] buf = new byte[65536];
                int read;
                while ((read = in.read(buf)) != -1) out.write(buf, 0, read);
                session.fsync(out);
            }

            Intent broadcastIntent = new Intent(InstallResultReceiver.ACTION)
                    .setPackage(context.getPackageName());
            PendingIntent pi = PendingIntent.getBroadcast(
                    context, sessionId, broadcastIntent,
                    PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);
            session.commit(pi.getIntentSender());
        }
    }

    private static void installInteractive(Context context, File apk) {
        Uri uri = FileProvider.getUriForFile(
                context,
                context.getPackageName() + ".fileprovider",
                apk);
        Intent intent = new Intent(Intent.ACTION_INSTALL_PACKAGE);
        intent.setData(uri);
        intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
    }

    static boolean hasSameSignature(Context context, File apk) {
        try {
            Signature[] installed = getSignatures(context.getPackageManager(), context.getPackageName(), false);
            Signature[] candidate = getApkSignatures(context.getPackageManager(), apk.getAbsolutePath());
            if (installed == null || candidate == null) return false;
            if (installed.length != candidate.length) return false;
            for (Signature sig : installed) {
                boolean found = false;
                for (Signature s : candidate) {
                    if (Arrays.equals(sig.toByteArray(), s.toByteArray())) { found = true; break; }
                }
                if (!found) return false;
            }
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    @SuppressWarnings("deprecation")
    private static Signature[] getSignatures(PackageManager pm, String packageName, boolean unused)
            throws PackageManager.NameNotFoundException {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            PackageInfo pi = pm.getPackageInfo(packageName, PackageManager.GET_SIGNING_CERTIFICATES);
            if (pi.signingInfo.hasMultipleSigners()) {
                return pi.signingInfo.getApkContentsSigners();
            } else {
                return pi.signingInfo.getSigningCertificateHistory();
            }
        } else {
            PackageInfo pi = pm.getPackageInfo(packageName, PackageManager.GET_SIGNATURES);
            return pi.signatures;
        }
    }

    @SuppressWarnings("deprecation")
    private static Signature[] getApkSignatures(PackageManager pm, String apkPath) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            PackageInfo pi = pm.getPackageArchiveInfo(apkPath, PackageManager.GET_SIGNING_CERTIFICATES);
            if (pi == null || pi.signingInfo == null) return null;
            if (pi.signingInfo.hasMultipleSigners()) {
                return pi.signingInfo.getApkContentsSigners();
            } else {
                return pi.signingInfo.getSigningCertificateHistory();
            }
        } else {
            PackageInfo pi = pm.getPackageArchiveInfo(apkPath, PackageManager.GET_SIGNATURES);
            if (pi == null) return null;
            return pi.signatures;
        }
    }
}
