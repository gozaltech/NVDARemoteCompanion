package org.gozaltech.nvdaremotecompanion.android;

import android.app.Application;
import android.content.ContentResolver;
import android.net.Uri;
import android.util.Log;

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
import java.util.concurrent.ExecutorService;
import java.util.function.Consumer;
import java.util.concurrent.Executors;

public class MainViewModel extends AndroidViewModel {

    private static final String TAG = "NVDARemote/ViewModel";

    private final ExecutorService executor = Executors.newCachedThreadPool();

    private final MutableLiveData<List<ProfileUiState>> profilesLive    = new MutableLiveData<>();
    private final MutableLiveData<String>               statusLive      = new MutableLiveData<>();
    private final MutableLiveData<Event<Integer>>       toastLive       = new MutableLiveData<>();
    private final MutableLiveData<Boolean>              sendingKeysLive = new MutableLiveData<>(false);

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

    public LiveData<List<ProfileUiState>> getProfiles()       { return profilesLive; }
    public LiveData<String>               getStatusSubtitle() { return statusLive; }
    public LiveData<Event<Integer>>       getToast()          { return toastLive; }
    public LiveData<Boolean>              getSendingKeys()    { return sendingKeysLive; }

    public void onServiceConnected(ConnectionService svc) {
        connectionService = svc;
        refreshProfiles();
    }

    public void onServiceDisconnected() {
        connectionService = null;
    }

    public void connect(int index) {
        if (connectionService == null) {
            postToast(R.string.error_connect_failed_generic);
            return;
        }
        connectionService.connect(index, ok -> {
            if (!ok) postToast(R.string.error_connect_failed_generic);
        });
    }

    public void disconnect(int index)          { withService(s -> s.disconnect(index)); }
    public void setActiveProfile(int index)    { withService(s -> s.setActiveProfile(index)); }
    public void toggleForwarding()             { withService(ConnectionService::toggleForwarding); }
    public void deleteProfile(int index)       { withService(s -> s.deleteProfile(index)); }
    public void setTtsEngine(String pkg)       { withService(s -> s.setTtsEngine(pkg)); }

    public void setScreenReaderMode(boolean use) {
        if (connectionService != null) connectionService.setScreenReaderMode(use);
        else AppPrefs.setScreenReaderMode(getApplication(), use);
    }

    public void setSpeechOutputMode(int mode) {
        AppPrefs.setSpeechOutputMode(getApplication(), mode);
        if (connectionService != null) connectionService.setSpeechOutputMode(mode);
    }

    public void setDirectTtsEngine(String engine) {
        AppPrefs.setDirectTtsEngine(getApplication(), engine);
        withService(s -> s.setDirectTtsEngine(engine));
    }

    public void setUseAccessibilityStream(boolean use) {
        if (connectionService != null) connectionService.setUseAccessibilityStream(use);
        else AppPrefs.setAccessibilityStream(getApplication(), use);
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
                Log.e(TAG, "Export failed", e);
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
                int added = NativeBridge.nativeMergeConfig(json);
                NativeBridge.syncConnectionStates();
                refreshProfiles();
                postToast(added > 0 ? R.string.import_success : R.string.import_no_new_profiles);
            } catch (Exception e) {
                Log.e(TAG, "Import failed", e);
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
            boolean connected = NativeBridge.nativeIsConnected(i);
            list.add(new ProfileUiState(i, displayName, connected, connected && activeProfile == i));
        }
        profilesLive.postValue(list);
        sendingKeysLive.postValue(NativeBridge.nativeIsSendingKeys());

        String status;
        if (NativeBridge.nativeIsSendingKeys()) {
            status = getApplication().getString(R.string.status_sending_to,
                    NativeBridge.nativeGetProfileName(activeProfile));
        } else if (activeProfile >= 0) {
            status = getApplication().getString(R.string.status_active_profile,
                    NativeBridge.nativeGetProfileName(activeProfile));
        } else {
            status = getApplication().getString(R.string.status_local);
        }
        statusLive.postValue(status);
    }

    private void withService(java.util.function.Consumer<ConnectionService> action) {
        if (connectionService != null) action.accept(connectionService);
    }

    private void postToast(int resId) {
        toastLive.postValue(new Event<>(resId));
    }
}
