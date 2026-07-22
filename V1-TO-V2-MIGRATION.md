# Migrating extension-gpgs projects from GPGS v1 to v2

This guide is for Defold projects upgrading to the extension's GPGS v2 major release. It focuses on changes required in the game project and Lua code; extension internals are not covered.

Google has deprecated the GPGS v1 SDK and is shutting down the v1 service. See Google's [Android migration guide](https://developer.android.com/games/pgs/android/migrate-to-v2) and [GPGS deprecation schedule](https://developer.android.com/games/pgs/deprecation) for the platform-level details.

This is a breaking extension update. Compatibility aliases for the old names are not provided.

## Changes at a glance

| GPGS v1-era project | GPGS v2 project | Required action |
| --- | --- | --- |
| `gpgs.COLLECTION_SOCIAL` | `gpgs.COLLECTION_FRIENDS` | Replace the constant; do not use the old numeric value `1`. |
| `gpgs.MSG_GET_SERVER_TOKEN` | `gpgs.MSG_GET_SERVER_AUTH_CODE` | Rename the callback message ID. |
| `message.token` | `message.code` | Read the auth code from the new field. |
| `gpgs.client_token` or `gpgs.client` in older documentation | `gpgs.client_id` | Configure the Web application OAuth client ID. |
| `gpgs.MSG_ACHIEVEMENTS` in older documentation | `gpgs.MSG_GET_ACHIEVEMENTS` | Use the exported achievement result message ID. |
| Explicit sign-in state management | Automatic GPGS v2 authentication plus an explicit sign-in fallback | Use `gpgs.silent_login()` to check the automatic state and `gpgs.login()` from a button when needed. |
| In-game sign-out | Not supported by GPGS v2 | Remove sign-out UI or logic. |
| Player IDs assumed to be shared across games | Next-generation Player IDs are scoped to one PGS project | Review backend identity assumptions and test account linking. |

## 1. Update the extension dependency

Replace the old extension dependency in `game.project` with the release URL for the GPGS v2 major version. Use a specific version from the [extension-gpgs releases](https://github.com/defold/extension-gpgs/releases) instead of `master` for production projects, then fetch libraries again in Defold.

If your project or another native extension directly adds the old `com.google.android.gms:play-services-games` or `play-services-auth` artifacts solely for the v1 integration, remove those dependencies. This extension supplies `play-services-games-v2` itself.

## 2. Update `game.project`

The extension's current Google Play services dependencies require Android 6.0 (API level 23) or newer:

```ini
[android]
minimum_sdk_version = 23

[gpgs]
app_id = 123456789012
use_saved_games = 1
request_server_auth_code = 0
```

The existing `app_id` remains your numeric Google Play Games Services project ID. `use_saved_games` remains optional and can be set to `0` if snapshots are not used.

If your backend needs a server auth code, change the flag and add the Web application OAuth client ID to the same `[gpgs]` section:

```ini
request_server_auth_code = 1
client_id = 123456789012-example.apps.googleusercontent.com
```

`client_id` must be the Web application OAuth client ID, not the Android OAuth client ID. The returned value is a short-lived, one-time server auth code intended to be sent to your backend; it is not an access token.

## 3. Update authentication code

GPGS v2 automatically attempts authentication when the game starts. Register the extension callback before checking the result:

```lua
local function gpgs_callback(self, message_id, message)
    if message_id == gpgs.MSG_SILENT_SIGN_IN or message_id == gpgs.MSG_SIGN_IN then
        if message.status == gpgs.STATUS_SUCCESS then
            print("Signed in as", gpgs.get_display_name())
        else
            print("Sign-in failed", message.error)
        end
    end
end

function init(self)
    if gpgs then
        gpgs.set_callback(gpgs_callback)
        gpgs.silent_login()
    end
end
```

Call `gpgs.login()` from a user action, such as a sign-in button, when silent authentication fails. Do not add a sign-out button: GPGS v2 no longer supports signing out from the game.

An ordinary extension project does not need to call `PlayGamesSdk.initialize()` or use the Android `GoogleSignInClient`; initialization and the v2 clients are managed by the extension and Google Play services.

## 4. Review Player ID assumptions

`gpgs.get_id()` returns the Google Play Games Services Player ID. With [next-generation Player IDs](https://developer.android.com/games/pgs/next-gen-player-ids), a new player receives a different ID for each game, while the ID remains consistent for that game across devices. Players who had already authenticated with the game retain their existing Player ID.

If your backend uses the Player ID as its account key, test that existing and new players still link to the correct game progress. Do not assume that `gpgs.get_id()` returns the same value across separate games. Follow Google's Player ID guidance before changing a production identity scheme.

## 5. Update server auth code handling

The result is asynchronous. Replace both the callback constant and payload field:

```lua
local function gpgs_callback(self, message_id, message)
    if message_id == gpgs.MSG_GET_SERVER_AUTH_CODE then
        if message.status == gpgs.STATUS_SUCCESS then
            local auth_code = message.code
            -- Send auth_code to your backend over your authenticated HTTPS connection.
        else
            print("Server auth code error", message.error)
        end
    end
end
```

The sign-in callback is delivered before `MSG_GET_SERVER_AUTH_CODE`. `gpgs.get_server_auth_code()` also returns the cached code after the successful auth-code callback; it can return `nil` before that operation completes.

## 6. Update leaderboard collections

Replace every use of `gpgs.COLLECTION_SOCIAL` with one of these v2 constants:

- `gpgs.COLLECTION_PUBLIC` for publicly visible scores.
- `gpgs.COLLECTION_FRIENDS` for scores from the current player's friends list.

Do not replace `COLLECTION_SOCIAL` with its old hard-coded value. GPGS v2 defines `COLLECTION_FRIENDS` as a different value.

Example:

```lua
gpgs.leaderboard_get_top_scores(
    "CgkIq5-gxcsVEAIQAg",
    gpgs.TIME_SPAN_ALL_TIME,
    gpgs.COLLECTION_FRIENDS,
    10
)
```

The first programmatic friends-score query may show Google's friends access consent UI. The extension resumes the original query once if consent is granted. If the player cancels or the resolution fails, the original leaderboard callback receives `gpgs.STATUS_FAILED`. See Google's [friends access guide](https://developer.android.com/games/pgs/android/friends).

## 7. Correct the achievement callback name

Older documentation referred to `gpgs.MSG_ACHIEVEMENTS`, but the exported result constant is:

```lua
if message_id == gpgs.MSG_GET_ACHIEVEMENTS then
    for _, encoded_achievement in ipairs(message) do
        local achievement = json.decode(encoded_achievement)
        print(achievement.id, achievement.name)
    end
end
```

The achievement, event, top-score, and player-centered-score callbacks continue to return arrays of JSON strings. Decode each element with `json.decode()`.

## 8. Test the migrated project

Before releasing, test a signed Android build using an account allowed to access the game in Google Play Console:

1. Bundle with `android.minimum_sdk_version` set to at least `23`.
2. Launch the game and verify the `MSG_SILENT_SIGN_IN` result.
3. Verify the explicit `gpgs.login()` flow after a silent sign-in failure.
4. Confirm existing and new test accounts link to the expected game progress.
5. Load a public leaderboard.
6. Load a friends leaderboard and test both accepting and cancelling the consent dialog.
7. Verify achievements, events, and snapshots used by the game.
8. If server authentication is enabled, send `message.code` to the backend and exchange it only once.

## Troubleshooting

### Sign-in fails

Check that the Android package name and signing certificate SHA-1 match the linked Android application in Google Play Console. Also verify that the account is an allowed tester and that `gpgs.app_id` is the numeric Play Games Services project ID.

### Server auth code retrieval fails

Verify that `request_server_auth_code` is `1` and `client_id` is the Web application OAuth client ID. Handle the error delivered with `MSG_GET_SERVER_AUTH_CODE`; do not expect a code in the sign-in callback.

### The friends leaderboard returns a failure

The player may have declined friends access. The failed callback contains `message.error` and may contain `message.error_status`. The game can retry the query later, but it must continue to handle failure while consent remains unavailable.

### Android dependency resolution fails

Search custom manifests and native extensions for direct v1 Google Play Games or Google Sign-In dependencies. Remove v1-only dependencies, then fetch libraries and bundle again.
