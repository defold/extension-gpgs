package com.defold.gpgs;

import android.app.Activity;
import android.content.Intent;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;

import com.google.android.gms.games.PlayGames;
import com.google.android.gms.games.GamesSignInClient;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.games.Player;
import com.google.android.gms.games.PlayersClient;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.android.gms.tasks.Task;

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
    // Request code for listing saved games
    private static final int RC_LIST_SAVED_GAMES = 9002;
    // Request code for listing achievements
    private static final int RC_ACHIEVEMENT_UI = 9003;
    private static final int RC_SHOW_LEADERBOARD = 9004;
    private static final int RC_SHOW_ALL_LEADERBOARDS = 9005;

    // duplicate of enums from gpgs_extension.h:
    private static final int MSG_SIGN_IN = 1;
    private static final int MSG_SILENT_SIGN_IN = 2;
    private static final int MSG_SHOW_SNAPSHOTS = 4;
    private static final int MSG_LOAD_SNAPSHOT = 5;
    private static final int MSG_SAVE_SNAPSHOT = 6;
    private static final int MSG_ACHIEVEMENTS = 7;
    private static final int MSG_GET_TOP_SCORES = 8;
    private static final int MSG_GET_PLAYER_CENTERED_SCORES = 9;
    private static final int MSG_GET_PLAYER_SCORE = 10;
    private static final int MSG_GET_EVENTS = 11;
    private static final int MSG_GET_SERVER_TOKEN = 12;

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
    private boolean isDiskActive;
    private boolean isRequestAuthCode;
    private boolean isSupported;
    private String mWebClientToken = null; // need for server auth token request
    private String mServerAuthCode = null; // can be non-null if isRequestAuthCode == true

    //--------------------------------------------------
    // Authorization

    private GamesSignInClient mSignInClient;
    private Player mPlayer;


    private OnFailureListener newOnFailureListener(final int messageId, final String message) {
        return e -> sendFailedMessage(messageId, message, e);
    }

    private OnSuccessListener<Intent> newOnSuccessListenerForIntent(final int requestCode) {
        return intent -> activity.startActivityForResult(intent, requestCode);
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

    public GpgsJNI(Activity activity, boolean isDiskActive, boolean isRequestAuthCode, String oauthToken) {
        this.activity = activity;
        this.isDiskActive = isDiskActive;
        this.isRequestAuthCode = isRequestAuthCode;
        if (isRequestAuthCode) {
            this.mWebClientToken = oauthToken;
        }

        this.isSupported = GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(activity) == ConnectionResult.SUCCESS;

        mSignInClient = PlayGames.getGamesSignInClient(activity);
        if (isDiskActive) {
            mPlayerSnapshotsClient = PlayGames.getSnapshotsClient(activity);
        }
        mLeaderboardsClient = PlayGames.getLeaderboardsClient(activity);
        mAchievementsClient = PlayGames.getAchievementsClient(activity);
        mEventsClient = PlayGames.getEventsClient(activity);
    }

    private void onConnected(final int msg) {
        // until we get player information
        mPlayer = null;
        onAccountChangedDisk();
        PlayersClient playersClient = PlayGames.getPlayersClient(activity);
        if (this.isRequestAuthCode) {
            mSignInClient.requestServerSideAccess(mWebClientToken, false)
                    .addOnCompleteListener(task -> {
                        if (task.isSuccessful()) {
                            mServerAuthCode = task.getResult();
                            sendSimpleMessage(MSG_GET_SERVER_TOKEN, "status", STATUS_SUCCESS, "token", mServerAuthCode);
                        } else {
                            sendFailedMessage(MSG_GET_SERVER_TOKEN, "Can't get server auth token", task.getException());
                        }
                    });
        }
        playersClient.getCurrentPlayer()
                .addOnCompleteListener(task -> {
                    if (task.isSuccessful()) {
                        mPlayer = task.getResult();
                        sendSimpleMessage(msg, "status", STATUS_SUCCESS);
                    } else {
                        sendFailedMessage(MSG_SIGN_IN, "There was a problem getting the player id!", task.getException());
                    }
                });
    }

    public void activityResult(int requestCode, int resultCode, Intent intent) {
         if (requestCode == RC_LIST_SAVED_GAMES) {
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
        mSignInClient.isAuthenticated().addOnCompleteListener(isAuthenticatedTask -> {
            boolean isAuthenticated =
                    (isAuthenticatedTask.isSuccessful() &&
                            isAuthenticatedTask.getResult().isAuthenticated());
            if (isAuthenticated) {
                onConnected(MSG_SILENT_SIGN_IN);
            } else {
                sendFailedMessage(MSG_SILENT_SIGN_IN, "Silent sign-in failed", isAuthenticatedTask.getException());
            }
        });
    }

    public void login() {
        mSignInClient.signIn()
                .addOnCompleteListener(task -> {
                    if (task.isSuccessful() && task.getResult().isAuthenticated()) {
                        onConnected(MSG_SIGN_IN);
                    } else {
                        sendFailedMessage(MSG_SIGN_IN, "Sign-in failed", task.getException());
                    }
                });
    }

    public String getDisplayName() {
        return isLoggedIn() ? mPlayer.getDisplayName() : null;
    }

    public String getId() {
        return isLoggedIn() ? mPlayer.getPlayerId() : null;
    }

    public String getServerAuthCode() {
        return mServerAuthCode;
    }

    public boolean isLoggedIn() {
        return mPlayer != null;
    }

    public boolean isSupported() {
        return isSupported;
    }

    //--------------------------------------------------
    // GoogleDrive (Snapshots)

    // Client used to interact with Google Snapshots.
    private SnapshotsClient mPlayerSnapshotsClient = null;
    private Snapshot mPlayerSnapshot = null;
    private byte[] currentPlayerSave = null;

    private Snapshot mConflictingSnapshot = null;
    private byte[] conflictingSave = null;

    // values from the official docs: https://developers.google.com/android/reference/com/google/android/gms/games/SnapshotsClient.html#getMaxCoverImageSize()
    private int maxCoverImageSize = 819200;
    // https://developers.google.com/android/reference/com/google/android/gms/games/SnapshotsClient.html#getMaxDataSize()
    private int maxDataSize = 3145728;

    private void onAccountChangedDisk() {
        if (this.isDiskActive) {
            mPlayerSnapshotsClient.getMaxCoverImageSize()
                    .addOnCompleteListener(task -> {
                        if (task.isSuccessful()) {
                            maxCoverImageSize = task.getResult();
                        }
                    });

            mPlayerSnapshotsClient.getMaxDataSize()
                    .addOnCompleteListener(task -> {
                        if (task.isSuccessful()) {
                            maxDataSize = task.getResult();
                        }
                    });
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

    private OnCompleteListener<SnapshotsClient.DataOrConflict<Snapshot>> getOnLoadCompleteListener() {
        return task -> {
            if (!task.isSuccessful()) {
                sendFailedMessage(MSG_LOAD_SNAPSHOT, "Error while opening Snapshot", task.getException());
            } else {
                SnapshotsClient.DataOrConflict<Snapshot> result = task.getResult();
                if (!result.isConflict()) {
                    mPlayerSnapshot = result.getData();
                    try {
                        currentPlayerSave = mPlayerSnapshot.getSnapshotContents().readFully();
                        sendSnapshotMetadataMessage(MSG_LOAD_SNAPSHOT, mPlayerSnapshot.getMetadata());
                    } catch (IOException e) {
                        sendFailedMessage(MSG_LOAD_SNAPSHOT, "Error while reading Snapshot", e);
                    } catch (NullPointerException e) {
                        sendFailedMessage(MSG_LOAD_SNAPSHOT, "Snapshot is null", e);
                    }
                } else {
                    SnapshotsClient.SnapshotConflict conflict = result.getConflict();
                    mPlayerSnapshot = conflict.getSnapshot();
                    mConflictingSnapshot = conflict.getConflictingSnapshot();
                    try {
                        currentPlayerSave = mPlayerSnapshot.getSnapshotContents().readFully();
                        conflictingSave = mConflictingSnapshot.getSnapshotContents().readFully();
                        sendConflictMessage(MSG_LOAD_SNAPSHOT, mPlayerSnapshot.getMetadata(),
                                mConflictingSnapshot.getMetadata(), conflict.getConflictId());
                    } catch (IOException e) {
                        sendFailedMessage(MSG_LOAD_SNAPSHOT, "Error while reading Snapshot or Conflict", e);
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
                .addOnCompleteListener(task -> {
                    if (task.isSuccessful()) {
                        mPlayerSnapshot = null;
                        currentPlayerSave = null;
                        sendSimpleMessage(MSG_SAVE_SNAPSHOT,
                                "status", STATUS_SUCCESS);
                    } else {
                        sendFailedMessage(MSG_SAVE_SNAPSHOT, "Failed to save a snapshot", task.getException());
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
        return currentPlayerSave;
    }

    public byte[] getConflictingSave() {
        return conflictingSave;
    }

    public String setSave(byte[] bytes) {
        if (mPlayerSnapshot != null) {
            mPlayerSnapshot.getSnapshotContents().writeBytes(bytes);
            currentPlayerSave = bytes;
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

    public void submitScore(String leaderboardId, double score) {
        mLeaderboardsClient.submitScore(leaderboardId, (long)score);
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
        mLeaderboardsClient.loadTopScores(leaderboardId, span, collection, maxResults)
            .addOnCompleteListener(task -> {
                if (task.isSuccessful()) {
                    LeaderboardsClient.LeaderboardScores scores = task.getResult().get();
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
                } else {
                    sendFailedMessage(MSG_GET_TOP_SCORES, "Unable to get top scores", task.getException());
                }
            });
    }

    public void loadPlayerCenteredScores(final String leaderboardId, int span, int collection, int maxResults, boolean forceReload) {
        mLeaderboardsClient.loadPlayerCenteredScores(leaderboardId, span, collection, maxResults, forceReload)
            .addOnCompleteListener(task -> {
                if (task.isSuccessful()) {
                    LeaderboardsClient.LeaderboardScores scores = task.getResult().get();
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
                } else {
                    Exception exception = task.getException();
                    sendFailedMessage(MSG_GET_PLAYER_CENTERED_SCORES, "Unable to get player centered scores", exception);
                }
            });
    }

    public void loadCurrentPlayerLeaderboardScore(final String leaderboardId, int span, int collection) {
        mLeaderboardsClient.loadCurrentPlayerLeaderboardScore(leaderboardId, span, collection)
            .addOnCompleteListener(task -> {
                if (task.isSuccessful()) {
                    LeaderboardScore score = task.getResult().get();
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
                } else {
                    Exception exception = task.getException();
                    sendFailedMessage(MSG_GET_PLAYER_SCORE, "Unable to get player scores", exception);
                }
            });
    }

    public void showLeaderboard(String leaderboardId, int span, int collection) {
        mLeaderboardsClient.getLeaderboardIntent(leaderboardId, span, collection)
                .addOnSuccessListener(newOnSuccessListenerForIntent(RC_SHOW_LEADERBOARD));
    }

    public void showAllLeaderboards() {
        mLeaderboardsClient.getAllLeaderboardsIntent()
                .addOnSuccessListener(newOnSuccessListenerForIntent(RC_SHOW_ALL_LEADERBOARDS));
    }

    //--------------------------------------------------
    // Achievements

    // Client used to interact with Achievements.
    private AchievementsClient mAchievementsClient = null;

    public void revealAchievement(String achievementId) {
        mAchievementsClient.reveal(achievementId);
    }

    public void unlockAchievement(String achievementId) {
        mAchievementsClient.unlock(achievementId);
    }

    public void incrementAchievement(String achievementId, int steps) {
        mAchievementsClient.increment(achievementId, steps);
    }

    public void setAchievement(String achievementId, int steps) {
        mAchievementsClient.setSteps(achievementId, steps);
    }

    public void showAchievements() {
        mAchievementsClient.getAchievementsIntent()
                .addOnSuccessListener(newOnSuccessListenerForIntent(RC_UNUSED));
    }

    public void getAchievements() {
        mAchievementsClient
                .load(false)
                .addOnCompleteListener(task -> {
                    if (task.isSuccessful()) {
                        AchievementBuffer buffer = task.getResult().get();
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
                    } else {
                        Exception exception = task.getException();
                        sendFailedMessage(MSG_ACHIEVEMENTS, "Unable to get achievements", exception);
                    }
                });
    }

    private EventsClient mEventsClient = null;

    public void incrementEvent(String eventId, int amount) {
        mEventsClient.increment(eventId, amount);
    }

    public void loadEvents() {
        mEventsClient
                .load(false)
                .addOnCompleteListener(task -> {
                    if (task.isSuccessful()) {
                        EventBuffer buffer = task.getResult().get();
                        String message = null;
                        try {
                            JSONArray result = new JSONArray();
                            for (Event e : buffer) {
                                JSONObject json = new JSONObject();
                                json.put("id", e.getEventId());
                                json.put("formatted_value", e.getFormattedValue());
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
                    } else {
                        Exception exception = task.getException();
                        sendFailedMessage(MSG_GET_EVENTS, "Unable to get events", exception);
                    }
                });
    }

}
