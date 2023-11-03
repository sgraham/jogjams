import os
import urllib.request
import yt_dlp

SELFDIR = os.path.abspath(os.path.dirname(__file__))
CACHEDIR = os.path.abspath(os.path.join(os.path.dirname(__file__), 'cache'))

ydl_opts = {
    'ffmpeg_location': os.path.join(SELFDIR, 'bin', 'ffmpeg'),
    'format': 'mp3/bestaudio/best',
    'postprocessors': [
        {
            'key': 'FFmpegExtractAudio',
            'preferredcodec': 'mp3',
        }
    ],
    'outtmpl': os.path.join(CACHEDIR, 'music', '%(id)s.%(ext)s'),
    'overwrites': False,
    'download_archive': os.path.join(CACHEDIR, 'archive.dat'),
    'color': 'never',
}


class TrackInfo:
    def __init__(self, id, title, url, duration):
        self.id = id
        self.title = title
        self.url = url
        self.duration = duration
        self.local_file_name = None

    def ensure_cached(self):
        with yt_dlp.YoutubeDL(ydl_opts) as ydl:
            error_code = ydl.download(self.url)
        if error_code != 0:
            self.local_file_name = None
            return False
        self.local_file_name = os.path.join(CACHEDIR, 'music', self.id + '.mp3')
        return True


class PlaylistInfo:
    def __init__(self, id, title, url, local_thumb_file_name, tracks):
        self.id = id
        self.title = title
        self.url = url
        self.local_thumb_file_name = local_thumb_file_name
        self.tracks = tracks

    @staticmethod
    def create(url):
        with yt_dlp.YoutubeDL(ydl_opts) as ydl:
            info = ydl.extract_info(url, download=False, process=False)
            tracks = []
            for track in info['entries']:
                tracks.append(
                    TrackInfo(track['id'], track['title'], track['url'], track['duration'])
                )
            thumb_fn = None
            thumb_url = info['thumbnails'][0]['url'] if info['thumbnails'] else None
            if thumb_url:
                thumb_fn = os.path.join(CACHEDIR, 'thumb', info['id'] + '.jpg')
                if not os.path.isdir(os.path.dirname(thumb_fn)):
                    os.makedirs(os.path.dirname(thumb_fn))
                if not os.path.exists(thumb_fn):
                    with open(thumb_fn, 'wb') as f:
                        f.write(urllib.request.urlopen(thumb_url).read())
            return PlaylistInfo(info['id'], info['title'], info['original_url'], thumb_fn, tracks)

def main():
    URL = 'https://music.youtube.com/playlist?list=OLAK5uy_lLTN5rDT7EE5e7FuJEtkum7V-y4ATo3mM'
    pi = PlaylistInfo.create(URL)
    print(pi.id)
    print(pi.title)
    print(pi.url)
    for i,t in enumerate(pi.tracks):
        print(i, t.id)
        print(i, t.title)
        print(i, t.url)
        print(i, t.duration)
        print("ensure_cached:", t.ensure_cached())

if __name__ == '__main__':
    main()
