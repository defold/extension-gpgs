package com.defold.gpgs;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.util.Log;
import android.view.Gravity;

import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInClient;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.games.Games;
import com.google.android.gms.games.GamesClient;
import com.google.android.gms.games.Player;
import com.google.android.gms.games.PlayersClient;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.android.gms.tasks.Task;

import com.google.android.gms.drive.Drive;
import com.google.android.gms.games.SnapshotsClient;
import com.google.android.gms.games.snapshot.SnapshotMetadata;
import com.google.android.gms.games.snapshot.SnapshotMetadataChange;
import com.google.android.gms.games.snapshot.Snapshot;
import com.google.android.gms.tasks.Continuation;
import com.google.android.gms.common.api.ApiException;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import java.io.IOException;

import org.json.JSONObject;
import org.json.JSONException;

public class GpgsJNI {
    //Internal constants:

    // Request code used to invoke sign in user interactions.
    private static final int RC_SIGN_IN = 9001;
    // Request code for listing saved games
    private static final int RC_LIST_SAVED_GAMES = 9002;
    // Request code for selecting a snapshot
    private static final int RC_SELECT_SNAPSHOT = 9003;
    // Request code for saving the game to a snapshot.
    private static final int RC_SAVE_SNAPSHOT = 9004;

    private static final int RC_LOAD_SNAPSHOT = 9005;

    //Duplicate of ENUMS from С:
    private static final int MSG_SIGN_IN = 1;
    private static final int MSG_SILENT_SIGN_IN = 2;
    private static final int MSG_SIGN_OUT = 3;
    private static final int MSG_SHOW_SNAPSHOTS = 4;
    private static final int MSG_LOAD_SNAPSHOT = 5;
    private static final int MSG_SAVE_SNAPSHOT = 5;

    private static final int STATUS_SUCCESS = 1;
    private static final int STATUS_FAILED = 2;
    private static final int STATUS_CREATE_NEW_SAVE = 3;
    private static final int STATUS_CONFLICT = 4;

    //--------------------------------------------------
    public static native void gpgsAddToQueue(int msg, String json);
    //--------------------------------------------------
    private Activity activity;
    private boolean is_disk_active;

    //--------------------------------------------------
    // Autorization

    private GoogleSignInAccount mSignedInAccount = null;
    private GoogleSignInOptions mSignInOptions;
    private GoogleSignInClient  mGoogleSignInClient;
    private Player mPlayer;
    private int mGravity = Gravity.TOP | Gravity.CENTER_HORIZONTAL;

    private void sendSimpleMessage(int msg, String key_1, int value_1) {
        String message = null;
        try {
            JSONObject obj = new JSONObject();
            obj.put(key_1, value_1);
            message = obj.toString();
        } catch(JSONException e) {
            message = "{ error:'Error while converting simple message to JSON: " + e.getMessage() + "'";
        }
        gpgsAddToQueue(msg, message);
    }

    private void sendSimpleMessage(int msg, String key_1, int value_1, String key_2, String value_2) {
        String message = null;
        try {
            JSONObject obj = new JSONObject();
            obj.put(key_1, value_1);
            obj.put(key_2, value_2);
            message = obj.toString();
        } catch(JSONException e) {
            message = "{ error:'Error while converting simple message to JSON: " + e.getMessage() + "'";
        }
        gpgsAddToQueue(msg, message);
    }

    private void sendSimpleMessage(int msg, String key_1, int value_1, String key_2, int value_2, String key_3, String value_3) {
        String message = null;
        try {
            JSONObject obj = new JSONObject();
            obj.put(key_1, value_1);
            obj.put(key_2, value_2);
            obj.put(key_3, value_3);
            message = obj.toString();
        } catch(JSONException e) {
            message = "{ error:'Error while converting simple message to JSON: " + e.getMessage() + "'";
        }
        gpgsAddToQueue(msg, message);
    }


    public GpgsJNI(Activity activity, boolean is_disk_active) {
        this.activity = activity;
        this.is_disk_active = is_disk_active;

        mGoogleSignInClient = GoogleSignIn.getClient(activity, getSignInOptions());
    }

    private void onAccountChanged(GoogleSignInAccount googleSignInAccount) {
        onAccountChangedDisk(googleSignInAccount);
    }

    private void onConnected(GoogleSignInAccount googleSignInAccount, final int msg) {
        if (mSignedInAccount != googleSignInAccount || mPlayer == null) {

            mSignedInAccount = googleSignInAccount;
            onAccountChanged(googleSignInAccount);
            PlayersClient playersClient = Games.getPlayersClient(activity, googleSignInAccount);
            playersClient.getCurrentPlayer()
                    .addOnSuccessListener(new OnSuccessListener<Player>() {
                        @Override
                        public void onSuccess(Player player) {
                            mPlayer = player;
                            sendSimpleMessage(msg,
                                    "status", STATUS_SUCCESS);
                        }
                    })
                    .addOnFailureListener(new OnFailureListener() {
                        @Override
                        public void onFailure(@NonNull Exception e) {
                            sendSimpleMessage(MSG_SIGN_IN,
                                    "status", STATUS_FAILED,
                                    "error",
                                    "There was a problem getting the player id!");
                        }
                    });
        }
        GamesClient gamesClient = Games.getGamesClient(activity, googleSignInAccount);
        gamesClient.setGravityForPopups(mGravity);
        gamesClient.setViewForPopups(activity.findViewById(android.R.id.content));

    }

    private GoogleSignInOptions getSignInOptions() {
        if (mSignInOptions == null) {
            if (is_disk_active) {
                mSignInOptions = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN)
                        .requestScopes(Drive.SCOPE_APPFOLDER)
                        .build();
            } else {
                mSignInOptions = GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN;
            }
        }
        return  mSignInOptions;
    }

    public void activityResult(int requestCode, int resultCode, Intent intent) {
        if (requestCode == RC_SIGN_IN) {
            if (intent != null) {
                Task<GoogleSignInAccount> task =
                        GoogleSignIn.getSignedInAccountFromIntent(intent);
                if (task.isSuccessful()) {
                    onConnected(task.getResult(), MSG_SIGN_IN);
                } else {
                    sendSimpleMessage(MSG_SIGN_IN, "status", STATUS_FAILED,
                        "error", "Sign-in failed");
                }
            } else {
                sendSimpleMessage(MSG_SIGN_IN, "status", STATUS_FAILED,
                    "error", "Sign-in failed. Intent do not exist.");
            }
        } else if(requestCode == RC_LIST_SAVED_GAMES) {
            if (intent != null) {
                if (intent.hasExtra(SnapshotsClient.EXTRA_SNAPSHOT_METADATA)) {
                    SnapshotMetadata snapshotMetadata =
                            intent.getParcelableExtra(SnapshotsClient.EXTRA_SNAPSHOT_METADATA);
                    sendSnapshotMetadataMessage(MSG_SHOW_SNAPSHOTS, snapshotMetadata);
                } else if (intent.hasExtra(SnapshotsClient.EXTRA_SNAPSHOT_NEW)) {
                    sendSimpleMessage(MSG_SHOW_SNAPSHOTS, "status", STATUS_CREATE_NEW_SAVE);
                }
            } else {
                // Error message
            }
        }
    }

    public void silentLogin() {
        this.activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                GoogleSignInAccount account = GoogleSignIn.getLastSignedInAccount(activity);
                if (GoogleSignIn.hasPermissions(account, getSignInOptions().getScopeArray())) {
                    onConnected(account, MSG_SILENT_SIGN_IN);
                } else {
                    mGoogleSignInClient.silentSignIn().addOnCompleteListener(activity,
                        new OnCompleteListener<GoogleSignInAccount>() {
                            @Override
                            public void onComplete(@NonNull Task<GoogleSignInAccount> task) {
                                if (task.isSuccessful()) {
                                    onConnected(task.getResult(), MSG_SILENT_SIGN_IN);
                                } else {
                                    sendSimpleMessage(MSG_SILENT_SIGN_IN,
                                            "status", STATUS_FAILED,
                                            "error",
                                            "Silent sign-in failed");
                                }
                            }
                        });
                }
            }
        });
    }

    public void login() {
        Intent intent = mGoogleSignInClient.getSignInIntent();
        this.activity.startActivityForResult(intent, RC_SIGN_IN);
    }

    public void logout() {
        mGoogleSignInClient.signOut().addOnCompleteListener(this.activity,
            new OnCompleteListener<Void>() {
                @Override
                public void onComplete(@NonNull Task<Void> task) {
                    mSignedInAccount = null;
                    mPlayer = null;
                    sendSimpleMessage(MSG_SIGN_OUT, "status", STATUS_SUCCESS);
                }
            });
    }

    public String getDisplayName() {
        if (mPlayer == null) {
            return null;
        }
        return mPlayer.getDisplayName();
    }

    public String getId() {
        if (mPlayer == null) {
            return null;
        }
        return mPlayer.getPlayerId();
    }

    public boolean isLoggedIn() {
        if (mPlayer == null) {
            return false;
        }
        return true;
    }

    public void setGravityForPopups(int gravity) {
        mGravity = gravity;
    }

    //--------------------------------------------------
    // GoogleDrive (Snapshots)

    // Client used to interact with Google Snapshots.
    private SnapshotsClient mPlayerSnapshotsClient = null;
    private Snapshot mPlayerSnapshot = null;
    private Snapshot mConflictingSnapshot = null;
    private byte[] currentplayerSave = null;
    private byte[] conflictingSave = null;

    // values from the official docs: https://developers.google.com/android/reference/com/google/android/gms/games/SnapshotsClient.html#getMaxCoverImageSize()
    private int maxCoverImageSize = 819200;
    // https://developers.google.com/android/reference/com/google/android/gms/games/SnapshotsClient.html#getMaxDataSize()
    private int maxDataSize  = 3145728;

    private void onAccountChangedDisk(GoogleSignInAccount googleSignInAccount) {
        if (this.is_disk_active) {
            if (mPlayerSnapshotsClient == null) {
                mPlayerSnapshotsClient = Games.getSnapshotsClient(activity, googleSignInAccount);
                mPlayerSnapshotsClient.getMaxCoverImageSize()
                        .addOnCompleteListener(new OnCompleteListener<Integer>() {
                            @Override
                            public void onComplete(@NonNull Task<Integer> task) {
                                if (task.isSuccessful()) {
                                    maxCoverImageSize = task.getResult();
                                }
                    }
                });

                mPlayerSnapshotsClient.getMaxDataSize()
                        .addOnCompleteListener(new OnCompleteListener<Integer>() {
                            @Override
                            public void onComplete(@NonNull Task<Integer> task) {
                                if (task.isSuccessful()) {
                                    maxDataSize = task.getResult();
                                }
                            }
                        });
            } else {
                mPlayerSnapshotsClient = Games.getSnapshotsClient(activity, googleSignInAccount);
            }
        }
    }

    private void addSnapshotMetadtaToJson(JSONObject json, String name, SnapshotMetadata metadata) throws JSONException {
        JSONObject obj = json;
        if (name != null) {
            obj = new JSONObject();
            json.put(name, obj);
        }
        obj.put("coverImageAspectRatio", metadata.getCoverImageAspectRatio());
        obj.put("coverImageUri", metadata.getCoverImageUri());
        obj.put("description", metadata.getDescription());
        obj.put("deviceName", metadata.getDeviceName());
        obj.put("lastModifiedTimestamp", metadata.getLastModifiedTimestamp());
        obj.put("playedTime", metadata.getPlayedTime());
        obj.put("progressValue", metadata.getProgressValue());
        obj.put("snapshotId", metadata.getSnapshotId());
        obj.put("uniqueName", metadata.getUniqueName());
    }
    
    private void sendSnapshotMetadataMessage(int msg, SnapshotMetadata metadata) {
        String message = null;
        try {
            JSONObject obj = new JSONObject();
            obj.put("status", STATUS_SUCCESS);
            addSnapshotMetadtaToJson(obj, "metadata", metadata);
            message = obj.toString();
        } catch(JSONException e) {
            message = "{ 'error':'Error while converting a metadata message to JSON: " + e.getMessage() +
                    "', 'status': '" + STATUS_FAILED + " }";
        }
        gpgsAddToQueue(msg, message);
    }

    private void sendConflictMessage(int msg, SnapshotMetadata metadata, SnapshotMetadata conflictMetadata) {
        String message = null;
        try {
            JSONObject obj = new JSONObject();
            obj.put("status", STATUS_CONFLICT);
            addSnapshotMetadtaToJson(obj, "metadata", metadata);
            addSnapshotMetadtaToJson(obj, "conflictMetadata", conflictMetadata);
            message = obj.toString();
        } catch(JSONException e) {
            message = "{ 'error':'Error while converting a metadata or a conflict metadata message to JSON: " + e.getMessage() +
                    "', 'status': '" + STATUS_FAILED + " }";
        }
        gpgsAddToQueue(msg, message);
    }

    public void showSavedGamesUI(String popupTitle, boolean allowAddButton,
                                 boolean allowDelete, int maxNumberOfSavedGamesToShow) {

        if (mPlayerSnapshotsClient == null) {
            sendSimpleMessage(MSG_SHOW_SNAPSHOTS,
                    "status", STATUS_FAILED,
                    "error",
                    "Can't start activity for showing saved games. You aren't logged in.");
            return;
        }

        Task<Intent> intentTask = mPlayerSnapshotsClient.getSelectSnapshotIntent(
                popupTitle, allowAddButton, allowDelete, maxNumberOfSavedGamesToShow);

        intentTask
            .addOnSuccessListener(new OnSuccessListener<Intent>() {
                @Override
                public void onSuccess(Intent intent) {
                        activity.startActivityForResult(intent, RC_LIST_SAVED_GAMES);
                    }
                 })
            .addOnFailureListener(new OnFailureListener() {
                @Override
                public void onFailure(@NonNull Exception e) {
                    sendSimpleMessage(MSG_SHOW_SNAPSHOTS,
                            "status", STATUS_FAILED,
                            "error",
                            "Can't start activity for showing saved games.");
                }
        });
    }

    public void loadSnapshot(String saveName, boolean createIfNotFound, int conflictPolicy) {
        int conflictResolutionPolicy = SnapshotsClient.RESOLUTION_POLICY_MOST_RECENTLY_MODIFIED;

        if (mPlayerSnapshotsClient == null) {
            sendSimpleMessage(MSG_LOAD_SNAPSHOT,
                    "status", STATUS_FAILED,
                    "error",
                    "Failed to open snapshot. You aren't logged in.");
            return;
        }
        mPlayerSnapshotsClient.open(saveName, createIfNotFound, conflictPolicy)
            .addOnCompleteListener(new OnCompleteListener<SnapshotsClient.DataOrConflict<Snapshot>>() {
                @Override
                public void onComplete(@NonNull Task<SnapshotsClient.DataOrConflict<Snapshot>> task) {
                    if (!task.isSuccessful()) {
                        int error_status_code = 0;
                        Exception e = task.getException();
                        if (e instanceof ApiException) {
                            ApiException apiException = (ApiException) e;
                            error_status_code = apiException.getStatusCode();
                        }
                            sendSimpleMessage(MSG_LOAD_SNAPSHOT,
                                    "status", STATUS_FAILED,
                                    "error_status", error_status_code,
                                    "error",
                                    "Error while opening Snapshot. " + e.toString()
                                    );
                    } else {
                        SnapshotsClient.DataOrConflict<Snapshot> result = task.getResult();
                        if (!result.isConflict()) {
                            mPlayerSnapshot = result.getData();
                            try {
                                currentplayerSave = mPlayerSnapshot.getSnapshotContents().readFully();
                                sendSnapshotMetadataMessage(MSG_LOAD_SNAPSHOT, mPlayerSnapshot.getMetadata());
                            } catch (IOException e) {
                                sendSimpleMessage(MSG_LOAD_SNAPSHOT,
                                        "status", STATUS_FAILED,
                                        "error",
                                        "Error while reading Snapshot." + e.toString());
                            }
                        } else {
                            SnapshotsClient.SnapshotConflict conflict = result.getConflict();
                            mPlayerSnapshot = conflict.getSnapshot();
                            mConflictingSnapshot = conflict.getConflictingSnapshot();
                            try {
                                currentplayerSave = mPlayerSnapshot.getSnapshotContents().readFully();
                                conflictingSave = mConflictingSnapshot.getSnapshotContents().readFully();
                                sendConflictMessage(MSG_LOAD_SNAPSHOT, mPlayerSnapshot.getMetadata(), mConflictingSnapshot.getMetadata());
                            } catch (IOException e) {
                                sendSimpleMessage(MSG_LOAD_SNAPSHOT,
                                        "status", STATUS_FAILED,
                                        "error",
                                        "Error while reading Snapshot." + e.toString());
                            }
                        }
                    }
                }
            });
    }

    public void saveAndCloseSnapshot(long playedTime, long progressValue, String description, byte[] coverImage) {
        SnapshotMetadataChange.Builder builder = new SnapshotMetadataChange.Builder();
        if (playedTime != -1) {
            builder.setPlayedTimeMillis(playedTime);
        }
        if (progressValue != -1) {
            builder.setProgressValue(progressValue);
        }
        if (description != null) {
            builder.setDescription(description);
        }
        if (coverImage != null) {
            Bitmap bitmap = BitmapFactory.decodeByteArray(coverImage, 0, coverImage.length);
            builder.setCoverImage(bitmap);
        }

        if (mPlayerSnapshot == null) {
            sendSimpleMessage(MSG_SAVE_SNAPSHOT,
                    "status", STATUS_FAILED,
                    "error",
                    "A snapshot wasn't opened.");
            return;
        }

        mPlayerSnapshotsClient.commitAndClose(mPlayerSnapshot, builder.build())
                .addOnCompleteListener(new OnCompleteListener<SnapshotMetadata>() {
            @Override
            public void onComplete(@NonNull Task<SnapshotMetadata> task) {
                if (task.isSuccessful()) {
                    mPlayerSnapshot = null;
                    currentplayerSave = null;
                    sendSimpleMessage(MSG_SAVE_SNAPSHOT,
                            "status", STATUS_SUCCESS);
                } else {
                    Exception e = task.getException();
                    sendSimpleMessage(MSG_SAVE_SNAPSHOT,
                            "status", STATUS_FAILED,
                            "error",
                            "Failed to save a snapshot. "+ e.toString());
                }
            }
        });
    }

    public byte[] getSave() {
        return currentplayerSave;
    }

    public byte[] getConflictingSave() {
        return conflictingSave;
    }

    public String setSave(byte[] bytes) {
        if (mPlayerSnapshot != null) {
            mPlayerSnapshot.getSnapshotContents().writeBytes(bytes);
            currentplayerSave = bytes;
            return null;
        }
        return "Can't write data to the snapshot. The snapshot wasn't open.";
    }

    public boolean isSnapshotOpened() {
        if (mPlayerSnapshot == null || mPlayerSnapshot.getSnapshotContents() == null) {
            return false;
        }
        return !mPlayerSnapshot.getSnapshotContents().isClosed();
    }

    public int getMaxCoverImageSize() {
        return maxCoverImageSize;
    }

    public int getMaxDataSize() {
        return maxDataSize;
    }
}
