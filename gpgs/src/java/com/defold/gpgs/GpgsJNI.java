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
import com.sun.org.apache.xpath.internal.operations.Bool;

import com.google.android.gms.drive.Drive;
import com.google.android.gms.games.SnapshotsClient;
import com.google.android.gms.games.snapshot.SnapshotMetadata;

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
    private static final int RC_SAVED_GAMES = 9009;

    //Duplicate of ENUMS from С:
    private static final int MSG_SIGN_IN = 1;
    private static final int MSG_SILENT_SIGN_IN = 2;
    private static final int MSG_SIGN_OUT = 3;

    private static final int STATUS_SUCCESS = 1;
    private static final int STATUS_FAILED = 2;
    //--------------------------------------------------
    public static native void gpgsAddToQueue(int msg, String key_1, int value_1, String key_2, String value_2);
    public static native void gpgsAddToQueueFirstArg(int msg, String key_1, int value_1);
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

    public GpgsJNI(Activity activity, boolean is_disk_active) {
        this.activity = activity;
        this.is_disk_active = is_disk_active;

        mGoogleSignInClient = GoogleSignIn.getClient(activity, getSignInOptions());
    }

    private void onConnected(GoogleSignInAccount googleSignInAccount, final int msg) {
        if (mSignedInAccount != googleSignInAccount || mPlayer == null) {

            mSignedInAccount = googleSignInAccount;

            PlayersClient playersClient = Games.getPlayersClient(activity, googleSignInAccount);
            playersClient.getCurrentPlayer()
                    .addOnSuccessListener(new OnSuccessListener<Player>() {
                        @Override
                        public void onSuccess(Player player) {
                            mPlayer = player;
                            gpgsAddToQueueFirstArg(msg,
                                    "status", STATUS_SUCCESS);
                        }
                    })
                    .addOnFailureListener(new OnFailureListener() {
                        @Override
                        public void onFailure(@NonNull Exception e) {
                            gpgsAddToQueue(MSG_SIGN_IN,
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

            Task<GoogleSignInAccount> task =
                    GoogleSignIn.getSignedInAccountFromIntent(intent);
            if (task.isSuccessful()) {
                onConnected(task.getResult(), MSG_SIGN_IN);
            } else {
                gpgsAddToQueue(MSG_SIGN_IN, "status", STATUS_FAILED,
                        "error", "Sign-in failed");
            }
        } else if(requestCode == RC_SAVED_GAMES) {
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
                                    gpgsAddToQueue(MSG_SILENT_SIGN_IN,
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
                    gpgsAddToQueueFirstArg(MSG_SIGN_OUT, "status", STATUS_SUCCESS);
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


    public void showSavedGamesUI(String popupTitle, boolean allowAddButton,
                                 boolean allowDelete, int maxNumberOfSavedGamesToShow) {
        SnapshotsClient snapshotsClient =
                Games.getSnapshotsClient(activity, GoogleSignIn.getLastSignedInAccount(activity));

        Task<Intent> intentTask = snapshotsClient.getSelectSnapshotIntent(
                popupTitle, allowAddButton, allowDelete, maxNumberOfSavedGamesToShow);

        intentTask.addOnSuccessListener(new OnSuccessListener<Intent>() {
            @Override
            public void onSuccess(Intent intent) {
                activity.startActivityForResult(intent, RC_SAVED_GAMES);
            }
        });
    }

}
