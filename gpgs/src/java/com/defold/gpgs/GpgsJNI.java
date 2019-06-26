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

public class GpgsJNI {
    //Internal constants:

    private static final int RC_SIGN_IN = 9001;

    private final static String TAG = "GPGS_DEFOLD";

    //Duplicate of ENUMS:
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

    //--------------------------------------------------
    private GoogleSignInAccount mSignedInAccount = null;
    private Player mPlayer;
    private int mGravity = Gravity.TOP | Gravity.CENTER_HORIZONTAL;

    private void onConnected(GoogleSignInAccount googleSignInAccount, final int msg) {
        if (mSignedInAccount != googleSignInAccount || mPlayer == null) {

            mSignedInAccount = googleSignInAccount;

            PlayersClient playersClient = Games.getPlayersClient(activity, googleSignInAccount);
            playersClient.getCurrentPlayer()
                    .addOnSuccessListener(new OnSuccessListener<Player>() {
                        @Override
                        public void onSuccess(Player player) {
                            mPlayer = player;
                            gpgsAddToQueueFirstArg(msg, "status", STATUS_SUCCESS);
                        }
                    })
                    .addOnFailureListener(new OnFailureListener() {
                        @Override
                        public void onFailure(@NonNull Exception e) {
                            Log.d(TAG, "There was a problem getting the player id! Code");
                        }
                    });
        }
        GamesClient gamesClient = Games.getGamesClient(activity, googleSignInAccount);
        gamesClient.setGravityForPopups(mGravity);
        gamesClient.setViewForPopups(activity.findViewById(android.R.id.content));

    }

    public GpgsJNI(Activity activity) {
        this.activity = activity;
    }

    public void activityResult(int requestCode, int resultCode, Intent intent) {
        Log.d(TAG, "activityResult: " + requestCode + " " + resultCode);
        if (requestCode == RC_SIGN_IN) {

            Task<GoogleSignInAccount> task =
                    GoogleSignIn.getSignedInAccountFromIntent(intent);
            if (task.isSuccessful()) {
                onConnected(task.getResult(), MSG_SIGN_IN);
            } else {
                gpgsAddToQueue(MSG_SIGN_IN, "status", STATUS_FAILED, "error", "Sign-in failed");
            }
        }
    }

    public void silentLogin() {
        this.activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                GoogleSignInOptions signInOptions = GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN;
                GoogleSignInAccount account = GoogleSignIn.getLastSignedInAccount(activity);
                if (GoogleSignIn.hasPermissions(account, signInOptions.getScopeArray())) {
                    onConnected(account, MSG_SILENT_SIGN_IN);
                } else {
                    // Haven't been signed-in before. Try the silent sign-in first.
                    GoogleSignInClient signInClient = GoogleSignIn.getClient(activity, signInOptions);
                    signInClient
                            .silentSignIn()
                            .addOnCompleteListener(activity,
                                    new OnCompleteListener<GoogleSignInAccount>() {
                                        @Override
                                        public void onComplete(@NonNull Task<GoogleSignInAccount> task) {
                                            if (task.isSuccessful()) {
                                                onConnected(task.getResult(), MSG_SILENT_SIGN_IN);
                                            } else {
                                                gpgsAddToQueue(MSG_SILENT_SIGN_IN, "status", STATUS_FAILED, "error", "Silent sign-in failed");
                                            }
                                        }
                                    });
                }
            }
        });
    }

    public void login() {
        GoogleSignInClient signInClient = GoogleSignIn.getClient(this.activity, GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN);
        Intent intent = signInClient.getSignInIntent();
        this.activity.startActivityForResult(intent, RC_SIGN_IN);
    }

    public void logout() {
        GoogleSignInClient signInClient = GoogleSignIn.getClient(this.activity,
                GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN);
        signInClient.signOut().addOnCompleteListener(this.activity,
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


}
