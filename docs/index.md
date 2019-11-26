---
layout: default
---

# Defold Google Play Game Services API documentation

This extension provides functions for interacting with Google Play Game Services. Supported on Android.

# Usage
To use this library in your Defold project, add the following URL to your <code class="inline-code-block">game.project</code> dependencies:

    https://github.com/defold/extension-gpgs/archive/master.zip

We recommend using a link to a zip file of a [specific release](https://github.com/defold/extension-gpgs/releases).

# Dependencies

The extension has the following dependencies:

    https://github.com/defold/android-base-extensions/releases/download/1.0.0/gps-games-17.0.0.zip
    https://github.com/defold/android-base-extensions/releases/download/1.0.0/gps-base-16.1.0.zip
    https://github.com/defold/android-base-extensions/releases/download/1.0.0/support-v4-27.0.2.zip

## Google App Setup
In order to use Google Play Game Services your application needs to be added to the Google Play store. It doesn't have to be published but it must be registered. Read more about how to sign up for and use the Google Play store in [https://support.google.com/googleplay/android-developer/answer/6112435](the official documentation).

Once the application is registered you also need to enable Google Play Game Services for the application. Follow the official documentation to [https://developers.google.com/games/services/console/enabling](enable Google Play Game Services).

## Defold Setup

### game.project
Add the following section into your game.project file:

    [gpgs]
    app_id = 1234567890
    use_saved_games = 1
    request_server_auth_code = 0
    request_id_token = 0

Where `app_id` is the 12 or 13 digit Application ID from the Google Play Console, found under Development Tools > Services & APIs and Google Play game services.</p>

Where `use_saved_games` indicates if the [https://developers.google.com/games/services/common/concepts/savedgames](Game Saves service) should be used (0 is disabled, 1 is enabled).</p>


## Source code

The source code is available on [GitHub](https://github.com/defold/extension-gpgs)


# API reference

{% include api_ref.md %}
