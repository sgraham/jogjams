## v1
- only supports YTM and Garmin
- attach watch by USB
- paste playlist url
- background watch for WPD MTP device popping up
- probably just make it a separate exe because py ext is going to be pain
  - sit in loop, comm over stdin/stdout


- mtphelper is flickering the cursor like crazy
  - open pipe and send commands maybe? :/


- yt_dlp:
  - get info for playlist on paste in background
  - then download songs as just cache/id.ext

  {
    playlists: {
      "https://playlist?url": {
        [
          5JeArVBC56s,   
          HHKXKH34GAE,
          ...
        ]
      },
    }
    tracks: {
      5JeArVBC56s: {
        "name": "Sweat",
      },
      HHKXKH34GAE: {
        "name": "Hush",
      },
    },
  }

  - on user sync, copy and rename to "%(index)03d - %(name)s.mp3"


- jogjams.key by email for payment
- pyupdater
- pyinstaller
- jogjams.com/api/check-update posts key/log if any, and returns to update URL
- windowed packaging nicely
- all exceptions, etc to logging

## v2

log in to ytm to select playlists nicely?



## some other early ideas below here

client app:

- oauth to ytm
- find playlist closest to containing "garmin"
- download content locally
- cache to cas blobstore (?)
- wxpython
- yt\_dlp
- WPD MTP local transfer instead of server/watch app
- simple/same for Coros that way




server:
- songid -> blob PUT/GET on simple vps
- a few days for cache expiry

watch:
- music provider garmin app
- could it sync via local server?
- or f the shitty apps and just do a local version with hackage to push to 

web service
- would probably be nicest to use
- would get banned for abuse quickly

chrome extension:
- a bit yikes

