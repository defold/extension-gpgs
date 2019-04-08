package com.defold.gpgs;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.util.Log;

import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInClient;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;

public class GpgsJNI {
    private static final int RC_SIGN_IN = 9001;
    final static String TAG = "GPGS_DEFOLD";

    private Activity activity;
    private GoogleSignInAccount signedInAccount;


    //--------------------------------------------------
    public GpgsJNI(Activity activity) {
        this.activity = activity;
    }

    public void silentLogin() {
        this.activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                GoogleSignInOptions signInOptions = GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN;
                GoogleSignInAccount account = GoogleSignIn.getLastSignedInAccount(activity);
                if (GoogleSignIn.hasPermissions(account, signInOptions.getScopeArray())) {
                    // Already signed in.
                    // The signed in account is stored in the 'account' variable.
                    signedInAccount = account;
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
                                                // The signed in account is stored in the task's result.
                                                signedInAccount = task.getResult();
                                                Log.d(TAG, "silent sign-in success");
                                            } else {
                                                Log.d(TAG, "silent sign-in failed");
                                                // Player will need to sign-in explicitly using via UI.
                                                // See [sign-in best practices](http://developers.google.com/games/services/checklist) for guidance on how and when to implement Interactive Sign-in,
                                                // and [Performing Interactive Sign-in](http://developers.google.com/games/services/android/signin#performing_interactive_sign-in) for details on how to implement
                                                // Interactive Sign-in.
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
        Log.d(TAG, "simple login tap");
    }

    private void logout() {
        GoogleSignInClient signInClient = GoogleSignIn.getClient(this.activity,
                GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN);
        signInClient.signOut().addOnCompleteListener(this.activity,
                new OnCompleteListener<Void>() {
                    @Override
                    public void onComplete(@NonNull Task<Void> task) {
                        Log.d(TAG, "logged out");
                    }
                });
    }


}
