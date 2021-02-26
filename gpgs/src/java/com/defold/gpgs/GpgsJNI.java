package com.defold.gpgs;

import android.app.Activity;
import android.content.Intent;
import androidx.annotation.NonNull;
import android.util.Log;
import android.view.Gravity;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInClient;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.games.Games;
import com.google.android.gms.games.GamesClient;
import com.google.android.gms.games.Player;
import com.google.android.gms.games.PlayersClient;
import com.google.android.gms.games.AnnotatedData;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.tasks.Continuation;

import com.google.android.gms.drive.Drive;
import com.google.android.gms.games.SnapshotsClient;
import com.google.android.gms.games.snapshot.SnapshotMetadata;
import com.google.android.gms.games.snapshot.SnapshotMetadataChange;
import com.google.android.gms.games.snapshot.Snapshot;

import com.google.android.gms.games.LeaderboardsClient;
import com.google.android.gms.games.leaderboard.LeaderboardScore;
import com.google.android.gms.games.leaderboard.LeaderboardScoreBuffer;

import com.google.android.gms.games.AchievementsClient;
import com.google.android.gms.games.achievement.Achievement;
import com.google.android.gms.games.achievement.AchievementBuffer;
import com.google.android.gms.games.GamesClientStatusCodes;

import com.google.android.gms.games.EventsClient;
import com.google.android.gms.games.event.Event;
import com.google.android.gms.games.event.EventBuffer;

import java.io.IOException;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

public class GpgsJNI {

    private static final String TAG = "GpgsJNI";

    //Internal constants:

    private static final int RC_UNUSED = 5001;
    // Request code used to invoke sign in user interactions.
    private static final int RC_SIGN_IN = 9001;
    // Request code for listing saved games
    private static final int RC_LIST_SAVED_GAMES = 9002;
    // Request code for listing achievements
    private static final int RC_ACHIEVEMENT_UI = 9003;
    private static final int RC_SHOW_LEADERBOARD = 9004;
    private static final int RC_SHOW_ALL_LEADERBOARDS = 9005;

    // duplicate of enums from gpgs_extension.h:
    private static final int MSG_SIGN_IN = 1;
    private static final int MSG_SILENT_SIGN_IN = 2;
    private static final int MSG_SIGN_OUT = 3;
    private static final int MSG_SHOW_SNAPSHOTS = 4;
    private static final int MSG_LOAD_SNAPSHOT = 5;
    private static final int MSG_SAVE_SNAPSHOT = 6;
    private static final int MSG_ACHIEVEMENTS = 7;
    private static final int MSG_GET_TOP_SCORES = 8;
    private static final int MSG_GET_PLAYER_CENTERED_SCORES = 9;
    private static final int MSG_GET_PLAYER_SCORE = 10;
    private static final int MSG_GET_EVENTS = 11;

    // duplicate of enums from gpgs_extension.h:
    private static final int STATUS_SUCCESS = 1;
    private static final int STATUS_FAILED = 2;
    private static final int STATUS_CREATE_NEW_SAVE = 3;
    private static final int STATUS_CONFLICT = 4;

    // duplicate of enums from gpgs_extension.h:
    private static final int SNAPSHOT_CURRENT = 1;
    private static final int SNAPSHOT_CONFLICTING = 2;

    //--------------------------------------------------
    public static native void gpgsAddToQueue(int msg, String json);

    //--------------------------------------------------
    private Activity activity;
    private boolean is_disk_active;
    private String client_id;
    private boolean is_request_id_token;
    private boolean is_request_auth_code;
    private boolean is_supported;

    //--------------------------------------------------
    // Authorization

    private GoogleSignInAccount mSignedInAccount = null;
    private GoogleSignInOptions mSignInOptions;
    private GoogleSignInClient mGoogleSignInClient;
    private Player mPlayer;
    private int mGravity = Gravity.TOP | Gravity.CENTER_HORIZONTAL;


    private OnFailureListener newOnFailureListener(final int messageId, final String message) {
        return new OnFailureListener() {
            @Override
            public void onFailure(@NonNull Exception e) {
                sendFailedMessage(messageId, message, e);
            }
        };
    }

    private OnSuccessListener<Intent> newOnSuccessListenerForIntent(final int requestCode) {
        return new OnSuccessListener<Intent>() {
            @Override
            public void onSuccess(Intent intent) {
                activity.startActivityForResult(intent, requestCode);
            }
        };
    }

    private void sendSimpleMessage(int msg, String key_1, int value_1) {
        String message = null;
        try {
            JSONObject obj = new JSONObject();
            obj.put(key_1, value_1);
            message = obj.toString();
        } catch (JSONException e) {
            message = "{ \"error\": \"Error while converting simple message to JSON: " + e.getMessage() + "\" }";
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
        } catch (JSONException e) {
            message = "{ \"error\": \"Error while converting simple message to JSON: " + e.getMessage() + "\" }";
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
        } catch (JSONException e) {
            message = "{ \"error\": \"Error while converting simple message to JSON: " + e.getMessage() + "\" }";
        }
        gpgsAddToQueue(msg, message);
    }

    private void sendFailedMessage(int msg, String error_text, Exception e) {
        if(e != null) {

            if (e instanceof ApiException) {
                ApiException apiException = (ApiException) e;
                Integer errorStatusCode = apiException.getStatusCode();
                error_text += ": " + GamesClientStatusCodes.getStatusCodeString(errorStatusCode) +" ("+errorStatusCode.toString()+")";

                sendSimpleMessage(msg,
                            "status", STATUS_FAILED,
                            "error_status", errorStatusCode,
                            "error", error_text);
            } else {
                error_text += ": " + e.toString();

                sendSimpleMessage(msg,
                    "status", STATUS_FAILED,
                    "error", error_text);
            }
        } else {

            sendSimpleMessage(msg,
                "status", STATUS_FAILED,
                "error", error_text);
        }
    }

    public GpgsJNI(Activity activity, boolean is_disk_active, boolean is_request_auth_code, boolean is_request_id_token, String client_id) {
        this.activity = activity;
        this.is_disk_active = is_disk_active;
        this.client_id = client_id;
        this.is_request_auth_code = is_request_auth_code;
        this.is_request_id_token = is_request_id_token;

        this.is_supported = GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(activity) == ConnectionResult.SUCCESS;

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
                    .addOnFailureListener(newOnFailureListener(MSG_SIGN_IN, "There was a problem getting the player id!"));
        }
        GamesClient gamesClient = Games.getGamesClient(activity, googleSignInAccount);
        gamesClient.setGravityForPopups(mGravity);
        gamesClient.setViewForPopups(activity.findViewById(android.R.id.content));

    }

    private GoogleSignInOptions getSignInOptions() {
        if (mSignInOptions == null) {
            GoogleSignInOptions.Builder builder = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN);

            if (is_disk_active) {
                builder.requestScopes(Drive.SCOPE_APPFOLDER);
            }

            if (is_request_id_token && client_id != null) {
                builder.requestIdToken(client_id);
            }

            if (is_request_auth_code && client_id != null) {
                builder.requestServerAuthCode(client_id);
            }

            mSignInOptions = builder.build();
        }

        return mSignInOptions;
    }

    public void activityResult(int requestCode, int resultCode, Intent intent) {
        if (requestCode == RC_SIGN_IN) {
            if (intent != null) {
                Task<GoogleSignInAccount> task =
                        GoogleSignIn.getSignedInAccountFromIntent(intent);

                if (task.isSuccessful()) {
                    onConnected(task.getResult(), MSG_SIGN_IN);
                } else {
                    sendFailedMessage(MSG_SIGN_IN, "Sign-in failed", task.getException());
                }
            } else {
                sendSimpleMessage(MSG_SIGN_IN, "status", STATUS_FAILED,
                        "error", "Sign-in failed. Intent do not exist.");
            }
        } else if (requestCode == RC_LIST_SAVED_GAMES) {
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
                                        sendFailedMessage(MSG_SILENT_SIGN_IN, "Silent sign-in failed", task.getException());
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
        return isLoggedIn() ? mPlayer.getDisplayName() : null;
    }

    public String getId() {
        return isLoggedIn() ? mPlayer.getPlayerId() : null;
    }

    public String getIdToken() {
        return isLoggedIn() ? mSignedInAccount.getIdToken() : null;
    }

    public String getServerAuthCode() {
        return isLoggedIn() ? mSignedInAccount.getServerAuthCode() : null;
    }

    public boolean isLoggedIn() {
        return mPlayer != null && mSignedInAccount != null;
    }

    public void setGravityForPopups(int gravity) {
        mGravity = gravity;
    }

    public boolean isSupported() {
        return is_supported;
    }

    //--------------------------------------------------
    // GoogleDrive (Snapshots)

    // Client used to interact with Google Snapshots.
    private SnapshotsClient mPlayerSnapshotsClient = null;
    private Snapshot mPlayerSnapshot = null;
    private byte[] currentplayerSave = null;

    private Snapshot mConflictingSnapshot = null;
    private byte[] conflictingSave = null;

    // values from the official docs: https://developers.google.com/android/reference/com/google/android/gms/games/SnapshotsClient.html#getMaxCoverImageSize()
    private int maxCoverImageSize = 819200;
    // https://developers.google.com/android/reference/com/google/android/gms/games/SnapshotsClient.html#getMaxDataSize()
    private int maxDataSize = 3145728;

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
        } catch (JSONException e) {
            message = "{ \"error\": \"Error while converting a metadata message to JSON: " + e.getMessage() + "\", \"status\": " + STATUS_FAILED + " }";
        }
        gpgsAddToQueue(msg, message);
    }

    private void sendConflictMessage(int msg, SnapshotMetadata metadata, SnapshotMetadata conflictMetadata, String conflictId) {
        String message = null;
        try {
            JSONObject obj = new JSONObject();
            obj.put("status", STATUS_CONFLICT);
            obj.put("conflictId", conflictId);
            addSnapshotMetadtaToJson(obj, "metadata", metadata);
            addSnapshotMetadtaToJson(obj, "conflictMetadata", conflictMetadata);
            message = obj.toString();
        } catch (JSONException e) {
            message = "{ \"error\": \"Error while converting a metadata or a conflict metadata message to JSON: " + e.getMessage() + "\", \"status\": " + STATUS_FAILED + " }";
        }
        gpgsAddToQueue(msg, message);
    }

    private OnCompleteListener getOnLoadCompleteListener() {
        return new OnCompleteListener<SnapshotsClient.DataOrConflict<Snapshot>>() {
            @Override
            public void onComplete(@NonNull Task<SnapshotsClient.DataOrConflict<Snapshot>> task) {
                if (!task.isSuccessful()) {
                    sendFailedMessage(MSG_LOAD_SNAPSHOT, "Error while opening Snapshot", task.getException());
                } else {
                    SnapshotsClient.DataOrConflict<Snapshot> result = task.getResult();
                    if (!result.isConflict()) {
                        mPlayerSnapshot = result.getData();
                        try {
                            currentplayerSave = mPlayerSnapshot.getSnapshotContents().readFully();
                            sendSnapshotMetadataMessage(MSG_LOAD_SNAPSHOT, mPlayerSnapshot.getMetadata());
                        } catch (IOException e) {
                            sendFailedMessage(MSG_LOAD_SNAPSHOT, "Error while reading Snapshot", e);
                        }
                    } else {
                        SnapshotsClient.SnapshotConflict conflict = result.getConflict();
                        mPlayerSnapshot = conflict.getSnapshot();
                        mConflictingSnapshot = conflict.getConflictingSnapshot();
                        try {
                            currentplayerSave = mPlayerSnapshot.getSnapshotContents().readFully();
                            conflictingSave = mConflictingSnapshot.getSnapshotContents().readFully();
                            sendConflictMessage(MSG_LOAD_SNAPSHOT, mPlayerSnapshot.getMetadata(),
                                    mConflictingSnapshot.getMetadata(), conflict.getConflictId());
                        } catch (IOException e) {
                            sendFailedMessage(MSG_LOAD_SNAPSHOT, "Error while reading Snapshot or Conflict", e);
                        }
                    }
                }
            }
        };
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
            .addOnSuccessListener(newOnSuccessListenerForIntent(RC_LIST_SAVED_GAMES))
            .addOnFailureListener(newOnFailureListener(MSG_SHOW_SNAPSHOTS, "Can't start activity for showing saved games"));
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
                .addOnCompleteListener(getOnLoadCompleteListener());
    }

    public void commitAndCloseSnapshot(long playedTime, long progressValue, String description, byte[] coverImage) {
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
                             sendFailedMessage(MSG_SAVE_SNAPSHOT, "Failed to save a snapshot", task.getException());
                        }
                    }
                });
    }

    public void resolveConflict(String conflictId, int metadataId) {
        Snapshot snapshot = mPlayerSnapshot;
        if (metadataId == SNAPSHOT_CONFLICTING) {
            snapshot = mConflictingSnapshot;
        }
        if (mPlayerSnapshot == null) {
            sendSimpleMessage(SNAPSHOT_CONFLICTING,
                    "status", STATUS_FAILED,
                    "error",
                    "Failed to resolve conflict. You aren't logged in.");
            return;
        }
        mPlayerSnapshotsClient.resolveConflict(conflictId, snapshot)
                .addOnCompleteListener(getOnLoadCompleteListener());
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


    //--------------------------------------------------
    // Leaderboards

    // Client used to interact with Leaderboards.
    private LeaderboardsClient mLeaderboardsClient = null;

    private boolean initLeaderboards() {
        if (mSignedInAccount == null) {
            return false;
        }
        if (mLeaderboardsClient == null) {
            mLeaderboardsClient = Games.getLeaderboardsClient(activity, mSignedInAccount);
        }
        return true;
    }

    public void submitScore(String leaderboardId, double score) {
        if(initLeaderboards()) {
            mLeaderboardsClient.submitScore(leaderboardId, (long)score);
        }
    }

    private static JSONObject scoreToJSON(LeaderboardScore score, String leaderboardId) throws JSONException {
        JSONObject json = new JSONObject();
        json.put("leaderboard_id", leaderboardId);
        json.put("display_rank", score.getDisplayRank());
        json.put("display_score", score.getDisplayScore());
        json.put("rank", score.getRank());
        json.put("score", score.getRawScore());
        json.put("tag", score.getScoreTag());
        json.put("timestamp", score.getTimestampMillis());
        json.put("score_holder_name", score.getScoreHolderDisplayName());
        json.put("score_holder_icon", score.getScoreHolderIconImageUri());
        json.put("score_holder_image", score.getScoreHolderHiResImageUri());
        return json;
    }

    public void loadTopScores(final String leaderboardId, int span, int collection, int maxResults) {
        if(initLeaderboards()) {
            Task<AnnotatedData<LeaderboardsClient.LeaderboardScores>> task = mLeaderboardsClient.loadTopScores(leaderboardId, span, collection, maxResults);
            task.addOnSuccessListener(new OnSuccessListener<AnnotatedData<LeaderboardsClient.LeaderboardScores>>() {
                @Override
                public void onSuccess(AnnotatedData<LeaderboardsClient.LeaderboardScores> data) {
                    LeaderboardsClient.LeaderboardScores scores = data.get();
                    LeaderboardScoreBuffer buffer = scores.getScores();
                    String message = null;
                    try {
                        JSONArray result = new JSONArray();
                        for (LeaderboardScore score : buffer) {
                            JSONObject json = scoreToJSON(score, leaderboardId);
                            result.put(json.toString());
                        }
                        message = result.toString();
                    } catch (JSONException e) {
                        message = "{ \"error\": \"Error while converting leaderboard score to JSON: " + e.getMessage() + "\", \"status\": " + STATUS_FAILED + " }";
                    }
                    buffer.release();
                    gpgsAddToQueue(MSG_GET_TOP_SCORES, message);
                }
            })
            .addOnFailureListener(newOnFailureListener(MSG_GET_TOP_SCORES, "Unable to get top scores"));
        }
    }

    public void loadPlayerCenteredScores(final String leaderboardId, int span, int collection, int maxResults) {
        if(initLeaderboards()) {
            Task<AnnotatedData<LeaderboardsClient.LeaderboardScores>> task = mLeaderboardsClient.loadPlayerCenteredScores(leaderboardId, span, collection, maxResults);
            task.addOnSuccessListener(new OnSuccessListener<AnnotatedData<LeaderboardsClient.LeaderboardScores>>() {
                @Override
                public void onSuccess(AnnotatedData<LeaderboardsClient.LeaderboardScores> data) {
                    LeaderboardsClient.LeaderboardScores scores = data.get();
                    LeaderboardScoreBuffer buffer = scores.getScores();
                    String message = null;
                    try {
                        JSONArray result = new JSONArray();
                        for (LeaderboardScore score : buffer) {
                            JSONObject json = scoreToJSON(score, leaderboardId);
                            result.put(json.toString());
                        }
                        message = result.toString();
                    } catch (JSONException e) {
                        message = "{ \"error\": \"Error while converting leaderboard score to JSON: " + e.getMessage() + "\", \"status\": " + STATUS_FAILED + " }";
                    }
                    buffer.release();
                    gpgsAddToQueue(MSG_GET_PLAYER_CENTERED_SCORES, message);
                }
            })
            .addOnFailureListener(newOnFailureListener(MSG_GET_PLAYER_CENTERED_SCORES, "Unable to get player centered scores"));
        }
    }

    public void loadCurrentPlayerLeaderboardScore(final String leaderboardId, int span, int collection) {
        if(initLeaderboards()) {
            Task<AnnotatedData<LeaderboardScore>> task = mLeaderboardsClient.loadCurrentPlayerLeaderboardScore(leaderboardId, span, collection);
            task.addOnSuccessListener(new OnSuccessListener<AnnotatedData<LeaderboardScore>>() {
                @Override
                public void onSuccess(AnnotatedData<LeaderboardScore> data) {
                    LeaderboardScore score = data.get();
                    String message = null;
                    if (score == null) {
                        message = "{ \"error\": \"Player has no score on leaderboard\", \"status\": " + STATUS_FAILED + " }";

                    }
                    else {
                        try {
                            JSONObject result = scoreToJSON(score, leaderboardId);
                            message = result.toString();
                        } catch (JSONException e) {
                            message = "{ \"error\": \"Error while converting leaderboard score to JSON: " + e.getMessage() + "\", \"status\": " + STATUS_FAILED + " }";
                        }
                    }
                    gpgsAddToQueue(MSG_GET_PLAYER_SCORE, message);
                }
            })
            .addOnFailureListener(newOnFailureListener(MSG_GET_PLAYER_SCORE, "Unable to get player scores"));
        }
    }

    public void showLeaderboard(String leaderboardId, int span, int collection) {
        if(initLeaderboards()) {
            mLeaderboardsClient.getLeaderboardIntent(leaderboardId, span, collection)
                .addOnSuccessListener(newOnSuccessListenerForIntent(RC_SHOW_LEADERBOARD));
        }
    }

    public void showAllLeaderboards() {
        if(initLeaderboards()) {
            mLeaderboardsClient.getAllLeaderboardsIntent()
                .addOnSuccessListener(newOnSuccessListenerForIntent(RC_SHOW_ALL_LEADERBOARDS));
        }
    }

    //--------------------------------------------------
    // Achievements

    // Client used to interact with Achievements.
    private AchievementsClient mAchievementsClient = null;

    private boolean initAchievements() {
        if (mSignedInAccount == null) {
            return false;
        }
        if (mAchievementsClient == null) {
            mAchievementsClient = Games.getAchievementsClient(activity, mSignedInAccount);
        }
        return true;
    }

    public void revealAchievement(String achievementId) {
        if(initAchievements()) {
            mAchievementsClient.reveal(achievementId);
        }
    }

    public void unlockAchievement(String achievementId) {
        if(initAchievements()) {
            mAchievementsClient.unlock(achievementId);
        }
    }

    public void incrementAchievement(String achievementId, int steps) {
        if(initAchievements()) {
            mAchievementsClient.increment(achievementId, steps);
        }
    }

    public void setAchievement(String achievementId, int steps) {
        if(initAchievements()) {
            mAchievementsClient.setSteps(achievementId, steps);
        }
    }

    public void showAchievements() {
        if(initAchievements()) {
            mAchievementsClient.getAchievementsIntent()
                .addOnSuccessListener(newOnSuccessListenerForIntent(RC_UNUSED));
        }
    }

    public void getAchievements() {
        if(initAchievements()) {
            Task<AnnotatedData<AchievementBuffer>> task = mAchievementsClient.load(false);
            task.addOnSuccessListener(new OnSuccessListener<AnnotatedData<AchievementBuffer>>() {
                @Override
                public void onSuccess(AnnotatedData<AchievementBuffer> data) {
                    AchievementBuffer buffer = data.get();
                    String message = null;
                    try {
                        JSONArray result = new JSONArray();
                        for (Achievement a : buffer) {
                            JSONObject json = new JSONObject();
                            json.put("id", a.getAchievementId());
                            json.put("name", a.getName());
                            json.put("description", a.getDescription());
                            json.put("xp", a.getXpValue());
                            if (a.getType() == Achievement.TYPE_INCREMENTAL) {
                                json.put("steps", a.getCurrentSteps());
                                json.put("total_steps", a.getTotalSteps());
                            }
                            if (a.getState() == Achievement.STATE_UNLOCKED) {
                                json.put("unlocked", true);
                            }
                            else if (a.getState() == Achievement.STATE_HIDDEN) {
                                json.put("hidden", true);
                            }
                            else if (a.getState() == Achievement.STATE_REVEALED) {
                                json.put("revealed", true);
                            }
                            result.put(json.toString());
                        }
                        message = result.toString();
                        buffer.release();
                    } catch (JSONException e) {
                        message = "{ \"error\": \"Error while converting achievements to JSON: " + e.getMessage() + "\", \"status\": " + STATUS_FAILED + " }";
                    }
                    gpgsAddToQueue(MSG_ACHIEVEMENTS, message);
                }
            })
            .addOnFailureListener(newOnFailureListener(MSG_ACHIEVEMENTS, "Unable to get achievements"));
        }
    }

    private EventsClient mEventsClient = null;

    private boolean initEvents() {
        if (mSignedInAccount == null) {
            return false;
        }
        if (mEventsClient == null) {
            mEventsClient = Games.getEventsClient(activity, mSignedInAccount);
        }
        return true;
    }

    public void incrementEvent(String eventId, int amount) {
        if(initEvents()) {
            mEventsClient.increment(eventId, amount);
        }
    }

    public void loadEvents() {
        if(initEvents()) {
            Task<AnnotatedData<EventBuffer>> task = mEventsClient.load(false);
            task.addOnSuccessListener(new OnSuccessListener<AnnotatedData<EventBuffer>>() {
                @Override
                public void onSuccess(AnnotatedData<EventBuffer> data) {
                    EventBuffer buffer = data.get();
                    String message = null;
                    try {
                        JSONArray result = new JSONArray();
                        for (Event e : buffer) {
                            JSONObject json = new JSONObject();
                            json.put("id", e.getEventId());
                            json.put("fomatted_value", e.getFormattedValue());
                            json.put("value", e.getValue());
                            json.put("description", e.getDescription());
                            json.put("image", e.getIconImageUri());
                            json.put("name", e.getName());
                            json.put("visible", e.isVisible());
                            result.put(json.toString());
                        }
                        message = result.toString();
                        buffer.release();
                    } catch (JSONException e) {
                        message = "{ \"error\": \"Error while converting event to JSON: " + e.getMessage() + "\", \"status\": " + STATUS_FAILED + " }";
                    }
                    gpgsAddToQueue(MSG_GET_EVENTS, message);
                }
            })
            .addOnFailureListener(newOnFailureListener(MSG_GET_EVENTS, "Unable to get events"));
        }
    }

}
