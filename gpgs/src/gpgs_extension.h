#pragma once

// https://developer.android.com/reference/android/view/Gravity.html
enum PopupPositions
{
    POPUP_POS_TOP_LEFT =           48 | 3,
    POPUP_POS_TOP_CENTER =         48 | 1,
    POPUP_POS_TOP_RIGHT =          48 | 5,
    POPUP_POS_CENTER_LEFT =        16 | 3,
    POPUP_POS_CENTER =             16 | 1,
    POPUP_POS_CENTER_RIGHT =       16 | 5,
    POPUP_POS_BOTTOM_LEFT =        80 | 3,
    POPUP_POS_BOTTOM_CENTER =      80 | 1,
    POPUP_POS_BOTTOM_RIGHT =       80 | 5
};

// https://developers.google.com/android/reference/com/google/android/gms/games/SnapshotsClient#RESOLUTION_POLICY_HIGHEST_PROGRESS
enum ResolutionPolicy
{
    RESOLUTION_POLICY_MANUAL =                    -1,
    RESOLUTION_POLICY_LONGEST_PLAYTIME =           1,
    RESOLUTION_POLICY_LAST_KNOWN_GOOD =            2,
    RESOLUTION_POLICY_MOST_RECENTLY_MODIFIED =     3,
    RESOLUTION_POLICY_HIGHEST_PROGRESS =           4
};

// https://developers.google.com/android/reference/com/google/android/gms/games/GamesClientStatusCodes.html
enum ERROR
{
    ERROR_STATUS_SNAPSHOT_NOT_FOUND =             26570,
    ERROR_STATUS_SNAPSHOT_CREATION_FAILED =       26571,
    ERROR_STATUS_SNAPSHOT_CONTENTS_UNAVAILABLE =  26572,
    ERROR_STATUS_SNAPSHOT_COMMIT_FAILED =         26573,
    ERROR_STATUS_SNAPSHOT_FOLDER_UNAVAILABLE =    26575,
    ERROR_STATUS_SNAPSHOT_CONFLICT_MISSING =      26576,
};

// https://developers.google.com/android/reference/com/google/android/gms/games/leaderboard/LeaderboardVariant#TIME_SPAN_DAILY
enum LeaderboardTimeSpan
{
    TIME_SPAN_DAILY = 0,
    TIME_SPAN_WEEKLY = 1,
    TIME_SPAN_ALL_TIME = 2
};

// https://developers.google.com/android/reference/com/google/android/gms/games/leaderboard/LeaderboardVariant#public-static-final-int-collection_public
enum LeaderboardCollection {
    COLLECTION_PUBLIC = 0,
    COLLECTION_SOCIAL = 1
};


// Internal to the extension
enum SNAPSHOT_TYPE
{
    SNAPSHOT_CURRENT =                1,
    SNAPSHOT_CONFLICTING =            2
};

// Internal to the extension
enum MESSAGE_ID
{
    MSG_SIGN_IN =                       1,
    MSG_SILENT_SIGN_IN =                2,
    MSG_SIGN_OUT =                      3,
    MSG_SHOW_SNAPSHOTS =                4,
    MSG_LOAD_SNAPSHOT =                 5,
    MSG_SAVE_SNAPSHOT =                 6,
    MSG_GET_ACHIEVEMENTS =              7,
    MSG_GET_TOP_SCORES =                8,
    MSG_GET_PLAYER_CENTERED_SCORES =    9,
    MSG_GET_PLAYER_SCORE =              10,
    MSG_GET_EVENTS =                    11
};

// Internal to the extension
enum STATUS
{
    STATUS_SUCCESS =                        1,
    STATUS_FAILED =                         2,
    STATUS_CREATE_NEW_SAVE =                3,
    STATUS_CONFLICT =                       4,
};
