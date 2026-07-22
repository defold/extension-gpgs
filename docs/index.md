---
title: Defold Google Play Games Services documentation
brief: This manual covers how to setup and use Google Play Games Services in Defold.
---

# Defold Google Play Games Services documentation

This extension provides functions for interacting with Google Play Games Services v2. Supported on Android. The extension supports the following services:

* Achievements
* Authentication
* Cloud save
* Events
* Leaderboards

Upgrading an existing project? Follow the [GPGS v1 to v2 migration guide](https://github.com/defold/extension-gpgs/blob/master/V1-TO-V2-MIGRATION.md).


## Installation
To use this library in your Defold project, add the following URL to your `game.project` dependencies:

https://github.com/defold/extension-gpgs/archive/master.zip

We recommend using a link to a zip file of a [specific release](https://github.com/defold/extension-gpgs/releases).


## Google App Setup
In order to use Google Play Games Services your application needs to be added to the Google Play store. It doesn't have to be published but it must be registered. Read more about how to sign up for and use the Google Play store in [the official documentation](https://support.google.com/googleplay/android-developer/answer/6112435).

Once the application is registered you also need to enable Google Play Games Services for the application. Follow the official documentation to [enable Google Play Games Services](https://developers.google.com/games/services/console/enabling).


## Defold Setup

The current Google Play services dependencies require Android 6.0 (API level 23) or newer. Set `android.minimum_sdk_version` to `23` or higher in `game.project`.

Add the following section into your `game.project` file (open and edit as a text file):

```
[gpgs]
app_id = 1234567890
use_saved_games = 1
request_server_auth_code = 0
```

Where `app_id` is the 12 or 13 digit Project ID from the Google Play Console. The Project ID can be found under "Grow users" > "Play Games Services" > "Setup and management" > "Configuration".

Where `use_saved_games` indicates if the [Game Saves service](https://developers.google.com/games/services/common/concepts/savedgames) should be used (0 is disabled, 1 is enabled).

If you want to retrieve a server auth code, add these keys to the same `[gpgs]` section. The client ID must belong to a Web application OAuth credential:

```
request_server_auth_code = 1
client_id = 1234567890-example.apps.googleusercontent.com
```

Authentication uses the GPGS v2 flow. Google Play Games automatically attempts authentication when the game starts; call `gpgs.silent_login()` to check that state or `gpgs.login()` to start the explicit sign-in flow. GPGS v2 does not support signing out from the game.

## Usage

The API uses a callback based system where events and data coming from Google Play Games Services are passed to the game client through a callback function. The kind of event coming from Google Play Games Services is identified by a pre-defined event id. Example:

```Lua
local function gpgs_callback(self, message_id, message)
    if message_id == gpgs.MSG_SIGN_IN then
        print("Signed in")
    end
end

gpgs.set_callback(gpgs_callback)
```

### Authentication:

```Lua

local function gpgs_callback(self, message_id, message)
    if message_id == gpgs.MSG_SIGN_IN or message_id == gpgs.MSG_SILENT_SIGN_IN then
        if message.status == gpgs.STATUS_SUCCESS then
            print("Signed in")
            print(gpgs.get_id())
            print(gpgs.get_display_name())
        else
            print("Sign in error!")
        end
    end
end

gpgs.set_callback(gpgs_callback)
gpgs.silent_login()

```


### Achievements

```Lua
gpgs.achievement_reveal("CgkIq5-gxcsVEAIQAQ")
gpgs.achievement_unlock("CgkIq5-gxcsVEAIQAQ")
gpgs.achievement_increment("CgkIq5-gxcsVEAIQAQ", 1)
gpgs.achievement_set("CgkIq5-gxcsVEAIQAQ", 10)
gpgs.achievement_show()
gpgs.achievement_get()
```


### Cloud save

```Lua
local function gpgs_callback(self, message_id, message)
    if message_id == gpgs.MSG_LOAD_SNAPSHOT then
        if message.status == gpgs.STATUS_CONFLICT then
            print(message.conflictId)
        end
    end
end


gpgs.set_callback(gpgs_callback)

gpgs.snapshot_display_saves("My saves", true, true, 3)
gpgs.snapshot_open("my_save", true, gpgs.RESOLUTION_POLICY_MANUAL)

local success, error_message = gpgs.snapshot_set_data("MyCustomBytesForSnapshot")

local bytes, error_message = gpgs.snapshot_get_data()
local bytes, error_message = gpgs.snapshot_get_conflicting_data()

gpgs.snapshot_resolve_conflict(self.conflictId, gpgs.SNAPSHOT_CURRENT)
gpgs.snapshot_resolve_conflict(self.conflictId, gpgs.SNAPSHOT_CONFLICTING)

gpgs.snapshot_commit_and_close({
    coverImage = screenshot.png(),
    description = "LEVEL 31, CAVE",
    playedTime = 1000,
    progressValue = 1234
})

gpgs.snapshot_is_opened()

```


### Events

```lua
local function gpgs_callback(self, message_id, message)
    if message_id == gpgs.MSG_GET_EVENTS then
        for _,event in ipairs(message) do
            event = json.decode(event)
            print(event.name, event.id, event.value)
        end
    end
end

gpgs.set_callback(gpgs_callback)
gpgs.event_increment("CgkIq5-gxcsVEAIQAw", 1)
gpgs.event_get()
```


### Leaderboards

```Lua

local function gpgs_callback(self, message_id, message)
    if message_id == gpgs.MSG_GET_PLAYER_SCORE then
        print(message.display_score, message.display_rank)
    elseif message_id == gpgs.MSG_GET_PLAYER_CENTERED_SCORES or message_id == gpgs.MSG_GET_TOP_SCORES then
        for _,score in ipairs(message) do
            score = json.decode(score)
            print(score.score_holder_name, score.display_score, score.display_rank)
        end
    end
end

gpgs.set_callback(gpgs_callback)

gpgs.leaderboard_submit_score("CgkIq5-gxcsVEAIQAg", 1337)
gpgs.leaderboard_get_top_scores("CgkIq5-gxcsVEAIQAg", gpgs.TIME_SPAN_ALL_TIME, gpgs.COLLECTION_PUBLIC, 10)
gpgs.leaderboard_get_player_centered_scores("CgkIq5-gxcsVEAIQAg", gpgs.TIME_SPAN_ALL_TIME, gpgs.COLLECTION_PUBLIC, 10)
gpgs.leaderboard_get_player_score("CgkIq5-gxcsVEAIQAg", gpgs.TIME_SPAN_ALL_TIME, gpgs.COLLECTION_PUBLIC)
gpgs.leaderboard_show("CgkIq5-gxcsVEAIQAg", gpgs.TIME_SPAN_ALL_TIME, gpgs.COLLECTION_PUBLIC)
```



## Source code

The source code is available on [GitHub](https://github.com/defold/extension-gpgs)
