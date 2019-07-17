# Google Play Game Services for Defold

## Google App Setup

You need to follow the guides on Google's pages to create a

https://developers.google.com/games/services/android/quickstart

## Platforms

The extension is only supported for Android.

## Defold setup

### Dependencies

You can use the GPGS extension in your own project by adding this project as a [Defold library dependency](http://www.defold.com/manuals/libraries/).
Open your game.project file and in the dependencies field under project add:

>https://github.com/defold/extension-gpgs/archive/master.zip

or point to the ZIP file of a [specific release](https://github.com/defold/extension-gpgs/releases).

### `game.project`
Add the following section into your `game.project` file:
```
[gpgs]
app_id = 1234567890
use_disk = 1
```
Where `app_id` app id from Google Play Console for your Play Services and `use_disk` option for using of the Game Saves (`0` is turned-off disk, `1` is turned-on disk).

## Signed .apk

To setup your app on google, you need to upload a signed package.
You can create a certificate using [this link](https://www.defold.com/manuals/android/#_creating_certificates_and_keys)


---

If you have any issues, questions or suggestions please [create an issue](https://github.com/defold/extension-gpgs/issues).
