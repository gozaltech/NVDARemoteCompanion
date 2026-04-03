package org.gozaltech.nvdaremotecompanion.android;

import android.app.Application;
import android.content.ContentResolver;
import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainViewModel extends AndroidViewModel {

    private final ExecutorService executor = Executors.newCachedThreadPool();

    private final MutableLiveData<List<ProfileUiState>> profilesLive = new MutableLiveData<>();
    private final MutableLiveData<String>               statusLive   = new MutableLiveData<>();
    private final MutableLiveData<Event<Integer>>       toastLive    = new MutableLiveData<>();

    private ConnectionService connectionService;

    private final NativeBridge.ConnectionStateListener stateListener =
            states -> refreshProfiles();

    public MainViewModel(@NonNull Application app) {
        super(app);
        NativeBridge.addConnectionStateListener(stateListener);
    }

    @Override
    protected void onCleared() {
        NativeBridge.removeConnectionStateListener(stateListener);
        executor.shutdownNow();
    }

    public LiveData<List<ProfileUiState>> getProfiles()     { return profilesLive; }
    public LiveData<String>               getStatusSubtitle() { return statusLive; }
    public LiveData<Event<Integer>>       getToast()        { return toastLive; }

    public void onServiceConnected(ConnectionService svc) {
        connectionService = svc;
        refreshProfiles();
    }

    public void onServiceDisconnected() {
        connectionService = null;
    }

    public void connect(int index, String displayName) {
        if (connectionService == null) {
            postToast(R.string.error_connect_failed_generic);
            return;
        }
        connectionService.connect(index, ok -> {
            if (!ok) postToast(R.string.error_connect_failed_generic);
        });
    }

    public void disconnect(int index) {
        if (connectionService != null) connectionService.disconnect(index);
    }

    public void setActiveProfile(int index) {
        if (connectionService != null) connectionService.setActiveProfile(index);
    }

    public void goLocal() {
        if (connectionService != null) connectionService.goLocal();
    }

    public void deleteProfile(int index) {
        if (connectionService != null) connectionService.deleteProfile(index);
    }

    public void setTtsEngine(String enginePackage) {
        if (connectionService != null) connectionService.setTtsEngine(enginePackage);
    }

    public void setScreenReaderMode(boolean use) {
        if (connectionService != null) connectionService.setScreenReaderMode(use);
        else AppPrefs.setScreenReaderMode(getApplication(), use);
    }

    public void setUseAccessibilityStream(boolean use) {
        if (connectionService != null) {
            connectionService.setUseAccessibilityStream(use);
        } else {
            AppPrefs.setAccessibilityStream(getApplication(), use);
        }
    }

    public void setTtsPitch(float value) {
        if (connectionService != null) connectionService.setTtsPitch(value);
        else AppPrefs.setTtsPitch(getApplication(), value);
    }

    public void setTtsRate(float value) {
        if (connectionService != null) connectionService.setTtsRate(value);
        else AppPrefs.setTtsRate(getApplication(), value);
    }

    public void setTtsVolume(float value) {
        if (connectionService != null) connectionService.setTtsVolume(value);
        else AppPrefs.setTtsVolume(getApplication(), value);
    }

    public void exportConfig(Uri uri, ContentResolver resolver) {
        executor.submit(() -> {
            try {
                String json = NativeBridge.nativeGetConfigJson();
                try (OutputStream os = resolver.openOutputStream(uri)) {
                    if (os != null) os.write(json.getBytes());
                }
                postToast(R.string.export_success);
            } catch (Exception e) {
                postToast(R.string.export_failed);
            }
        });
    }

    public void importConfig(Uri uri, ContentResolver resolver) {
        executor.submit(() -> {
            try {
                StringBuilder sb = new StringBuilder();
                try (BufferedReader br = new BufferedReader(
                        new InputStreamReader(resolver.openInputStream(uri)))) {
                    String line;
                    while ((line = br.readLine()) != null) sb.append(line).append('\n');
                }
                String json = sb.toString();
                new JSONObject(json);
                NativeBridge.nativeLoadConfig(json);
                refreshProfiles();
                postToast(R.string.import_success);
            } catch (Exception e) {
                postToast(R.string.import_failed);
            }
        });
    }

    private void refreshProfiles() {
        int count = NativeBridge.nativeGetProfileCount();
        int activeProfile = NativeBridge.nativeGetActiveProfile();

        List<ProfileUiState> list = new ArrayList<>(count);
        for (int i = 0; i < count; i++) {
            String name = NativeBridge.nativeGetProfileName(i);
            String displayName = name.isEmpty()
                    ? getApplication().getString(R.string.app_name) : name;
            list.add(new ProfileUiState(i, displayName, NativeBridge.nativeIsConnected(i), activeProfile == i));
        }
        profilesLive.postValue(list);

        String status = activeProfile >= 0
                ? getApplication().getString(R.string.status_sending_to,
                        NativeBridge.nativeGetProfileName(activeProfile))
                : getApplication().getString(R.string.status_local);
        statusLive.postValue(status);
    }

    private void postToast(int resId) {
        toastLive.postValue(new Event<>(resId));
    }
}
