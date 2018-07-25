# extension-gpgs

# Google App Setup

You need to follow the guides on Google's pages to create a

https://developers.google.com/games/services/android/quickstart

# Defold App setup

To get this extension to work with a google app, you will need to do a few steps:

* Create a folder named `<project>/res/android` and put the `google-services.json` there:
```bash
	res/android/google-services.json
```
* Put a reference to the `res/`folder in the `game.project` file:
```ini
[project]
bundle_resources = /res
```

* Merge the contents of the [app manifest](./game.appmanifest) with your app
* If you have no app manifest, simply copy this file, and refer to it in your `game.project`:
```ini
[native_extension]
app_manifest = /game.appmanifest
```

* Modify your `AndroidManifest.xml` to contain these lines:
```xml
   <meta-data android:name="com.google.android.gms.games.APP_ID"
        android:value="@string/app_id" />
   <meta-data android:name="com.google.android.gms.version"
       android:value="@integer/google_play_services_version"/>
```

* Put your game's resources in `res/android/values/`:
```bash
	res/android/values/ids.xml
	res/android/values/games-ids.xml
```

# Platforms

The extension is currently only supported for Android

# Tips

## Signed .apk

To setup your app on google, you need to upload a signed package.
You can create a certificate using [this link](https://forum.defold.com/t/how-do-i-get-a-private-key-and-certificate-for-a-release/18384/9)


# Extension Developers

[C++ readme](https://developers.google.com/games/services/cpp/GettingStartedNativeClient)
To update this extension to a newer version of GPGS you'll need to get the code from the [downloads page](https://developers.google.com/games/services/downloads/sdks)

You'll copy these files to the extension:
```bash
gpg-cpp-sdk/android/lib/c++/armeabi-v7a/libgpg.a
gpg-cpp-sdk/android/include/
```

Since we remove the old gpgs .jar files by using the app manifest, we also need updated resource files.
I've taken them from a GPG sample app created with Android Studio.
Put these in `gpgs/res/android`



