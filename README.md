# Jog Jams

## v1

- rip obvious junk out of wpd-mtp-hlper
- find users

## v2

- ERRORs from yt_dlp are a bit janky, maybe just wrap generically and log
- crashes on shutdown in w10 1607 vm?
- log into ytm to select playlists more nicely than c&p
- save recent playlists (just name+url in list?)
- jogjams.key by email for payment

### updater
- pyupdater
  - jogjams.com/api/check-update with version/key/log and returns update if any
  - seems janky and is unmaintained

- at startup, start thread that queries server with version for "should i
  update"?
- if so downloads to LOCALAPPDATA\Application\
  - make a jogjamssetup like chrome?
  - make entry point autoget if frozen and switcheroo o
  exit?


## v3+

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

