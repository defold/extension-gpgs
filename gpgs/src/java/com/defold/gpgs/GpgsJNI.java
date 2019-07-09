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
import com.google.android.gms.games.snapshot.Snapshot;
import com.google.android.gms.tasks.Continuation;
import com.google.android.gms.common.api.ApiException;

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

    private static final int STATUS_SUCCESS = 1;
    private static final int STATUS_FAILED = 2;
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

    private void SendSimpleMessage(int msg, String key_1, int value_1, String key_2, String value_2) {
        String message = null;
        try {
            JSONObject obj = new JSONObject();
            obj.put(key_1, value_1);
            if (key_2 != null)
            {
                obj.put(key_2, value_2);
            }
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
        if (this.is_disk_active) {
            mSnapshotsClient = Games.getSnapshotsClient(activity, googleSignInAccount);
        }
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
                            SendSimpleMessage(msg,
                                    "status", STATUS_SUCCESS, null, null);
                        }
                    })
                    .addOnFailureListener(new OnFailureListener() {
                        @Override
                        public void onFailure(@NonNull Exception e) {
                            SendSimpleMessage(MSG_SIGN_IN,
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
                    SendSimpleMessage(MSG_SIGN_IN, "status", STATUS_FAILED,
                        "error", "Sign-in failed");
                }
            } else {
                SendSimpleMessage(MSG_SIGN_IN, "status", STATUS_FAILED,
                    "error", "Sign-in failed. Intent do not exist.");
            }
        } else if(requestCode == RC_LIST_SAVED_GAMES) {
            if (intent != null) {
                if (intent.hasExtra(SnapshotsClient.EXTRA_SNAPSHOT_METADATA)) {
                    SnapshotMetadata snapshotMetadata =
                            intent.getParcelableExtra(SnapshotsClient.EXTRA_SNAPSHOT_METADATA);
                    //mCurrentSaveName = snapshotMetadata.getUniqueName();

                    // Load the game data from the Snapshot
                    // ...
                } else if (intent.hasExtra(SnapshotsClient.EXTRA_SNAPSHOT_NEW)) {
                    // Create a new snapshot named with a unique string
                    //String unique = new BigInteger(281, new Random()).toString(13);
                    //mCurrentSaveName = "snapshotTemp-" + unique;

                    // Create the new snapshot
                    // ...
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
                                    SendSimpleMessage(MSG_SILENT_SIGN_IN,
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
                    SendSimpleMessage(MSG_SIGN_OUT, "status", STATUS_SUCCESS, null, null);
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

    public boolean isAuthorized() {
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
    private SnapshotsClient mSnapshotsClient = null;
    private Snapshot mSnapshot = null;

    public void showSavedGamesUI(String popupTitle, boolean allowAddButton,
                                 boolean allowDelete, int maxNumberOfSavedGamesToShow) {

        Task<Intent> intentTask = mSnapshotsClient.getSelectSnapshotIntent(
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
                    SendSimpleMessage(MSG_SHOW_SNAPSHOTS,
                            "status", STATUS_FAILED,
                            "error",
                            "Can't start activity for showing saved games.");
                }
        });
    }

    public void loadSnapshot(String saveName, boolean createIfNotFound, int conflictPolicy) {
        int conflictResolutionPolicy = SnapshotsClient.RESOLUTION_POLICY_MOST_RECENTLY_MODIFIED;

        mSnapshotsClient.open(saveName, createIfNotFound, conflictPolicy)
            .continueWith(new Continuation<SnapshotsClient.DataOrConflict<Snapshot>, byte[]>() {
                @Override
                public byte[] then(@NonNull Task<SnapshotsClient.DataOrConflict<Snapshot>> task) throws Exception {
                    mSnapshot = task.getResult().getData();
                    try {
                        return mSnapshot.getSnapshotContents().readFully();
                    } catch (IOException e) {
                        SendSimpleMessage(MSG_LOAD_SNAPSHOT,
                            "status", STATUS_FAILED,
                            "error",
                            "Error while reading Snapshot." + e.toString());
                    }
                    return null;
                }
            }).addOnCompleteListener(new OnCompleteListener<byte[]>() {
                @Override
                public void onComplete(@NonNull Task<byte[]> task) {
                    if (task.isSuccessful()) {
                        Log.e("GPGS", "LOADED");
                        byte[] result = task.getResult();
                    } else {
                        int status = STATUS_FAILED;
                        Exception e = task.getException();
                        if (e instanceof ApiException) {
                            ApiException apiException = (ApiException) e;
                            status = apiException.getStatusCode();
                        }
                        SendSimpleMessage(MSG_LOAD_SNAPSHOT,
                                "status", status,
                                "error",
                                "Error while opening Snapshot. " + e.toString());
                    }
                }
            });
    }

}
