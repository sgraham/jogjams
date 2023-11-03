## v1

- pyinstaller
- pyupdater
  - jogjams.com/api/check-update with version/key/log and returns update if any
- no consoles
- redirect all logs
- rip obvious junk out of wpd-mtp-hlper
- find users
- ERRORs from yt_dlp are a bit janky, probably just wrap generically and log
- cache to USERAPPDIR or wherever it's supposed to be
- suggestion urls when no valid url

## v2

- jogjams.key by email for payment
- log into ytm to select playlists more nicely than c&p
- save recent playlists (just name+url in list?)
- jogjams.key by email for payment

## v-future

- watch app for wireless sync
- coros support
- other service support



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

