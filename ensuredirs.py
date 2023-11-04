import os
import platformdirs

_appname = 'JogJams'
_authorname = 'scot.tg'

CACHEDIR = platformdirs.user_cache_dir(_appname, _authorname)
if not os.path.isdir(CACHEDIR):
    os.makedirs(CACHEDIR)

LOGDIR = platformdirs.user_log_dir(_appname, _authorname)
if not os.path.isdir(LOGDIR):
    os.makedirs(LOGDIR)
