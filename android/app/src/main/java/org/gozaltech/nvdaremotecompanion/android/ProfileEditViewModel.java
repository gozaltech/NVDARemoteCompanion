package org.gozaltech.nvdaremotecompanion.android;

import android.util.Log;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import org.json.JSONObject;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ProfileEditViewModel extends ViewModel {

    private static final String TAG = "NVDARemote/ProfileEdit";

    private final ExecutorService executor = Executors.newCachedThreadPool();

    private final MutableLiveData<ProfileFormData> formDataLive  = new MutableLiveData<>();
    private final MutableLiveData<Event<Integer>>  toastLive     = new MutableLiveData<>();
    private final MutableLiveData<Event<Void>>     finishLive    = new MutableLiveData<>();

    @Override
    protected void onCleared() {
        executor.shutdownNow();
    }

    public LiveData<ProfileFormData> getFormData() { return formDataLive; }
    public LiveData<Event<Integer>>  getToast()    { return toastLive; }
    public LiveData<Event<Void>>     getFinish()   { return finishLive; }

    public void loadProfile(int index) {
        executor.submit(() -> {
            try {
                JSONObject p = ProfileRepository.getProfile(index);
                formDataLive.postValue(new ProfileFormData(
                        p.optString("name"),
                        p.optString("host"),
                        p.optInt("port", 6837),
                        p.optString("key"),
                        p.optBoolean("speech", true),
                        p.optBoolean("forward_nvda_sounds", true),
                        p.optBoolean("mute_on_local_control", false),
                        p.optBoolean("auto_connect", true)
                ));
            } catch (Exception e) {
                Log.e(TAG, "Failed to load profile " + index, e);
                toastLive.postValue(new Event<>(R.string.error_loading_profile));
            }
        });
    }

    public void save(int profileIndex, String name, String host, String portStr, String key,
                     boolean speech, boolean sounds, boolean mute, boolean autoConnect) {
        if (host.isEmpty() || key.isEmpty()) {
            toastLive.setValue(new Event<>(R.string.error_host_key_required));
            return;
        }
        int port;
        try { port = Integer.parseInt(portStr); } catch (NumberFormatException e) { port = 6837; }

        int finalPort = port;
        executor.submit(() -> {
            try {
                ProfileRepository.saveProfile(profileIndex, name, host, finalPort, key,
                        speech, sounds, mute, autoConnect);
                toastLive.postValue(new Event<>(R.string.profile_saved));
                finishLive.postValue(new Event<>(null));
            } catch (Exception e) {
                Log.e(TAG, "Failed to save profile " + profileIndex, e);
                toastLive.postValue(new Event<>(R.string.error_saving_profile));
            }
        });
    }

    public void delete(int profileIndex) {
        executor.submit(() -> {
            ProfileRepository.deleteProfile(profileIndex);
            toastLive.postValue(new Event<>(R.string.profile_deleted));
            finishLive.postValue(new Event<>(null));
        });
    }
}
