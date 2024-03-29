import ctypes

ctypes.windll.shcore.SetProcessDpiAwareness(True)

import ensuredirs
LOGDIR = ensuredirs.LOGDIR
CACHEDIR = ensuredirs.CACHEDIR


import os
SELFDIR = os.path.abspath(os.path.dirname(__file__))

import logging

logging.basicConfig(
    format='%(asctime)s:%(levelname)s:%(message)s',
    level=logging.DEBUG,
    filename=os.path.join(LOGDIR, 'jogjams.log'),
    encoding='utf-8',
)


import json
import localcache
import os
import re
import subprocess
import threading
import time
import urllib.request
import wx
import wx.adv
import yt_dlp

WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT = '{99ED0160-17FF-4C44-9D98-1D7A6F941921}'
WPD_CONTENT_TYPE_FOLDER = '{27E2E392-A111-48E0-AB0C-E17705A05F85}'
WPD_CONTENT_TYPE_AUDIO = '{4AD2C85E-5E2D-45E5-8864-4F229E3C6CF0}'

STATE_PLAYLIST_LOADED = 0
STATE_WATCH_CONNECTED = 1


class LogPipe(threading.Thread):
    def __init__(self, level):
        threading.Thread.__init__(self)
        self.daemon = False
        self.level = level
        self.fd_read, self.fd_write = os.pipe()
        self.pipe_reader = os.fdopen(self.fd_read)
        self.start()

    def fileno(self):
        return self.fd_write

    def run(self):
        for line in iter(self.pipe_reader.readline, ''):
            logging.log(self.level, line.strip('\n'))

        self.pipe_reader.close()

    def close(self):
        os.close(self.fd_write)


class ProportionalSplitter(wx.SplitterWindow):
    def __init__(self, parent, id=-1, proportion=0.5, size=wx.DefaultSize, **kwargs):
        wx.SplitterWindow.__init__(self, parent, id, wx.Point(0, 0), size, **kwargs)
        self.SetMinimumPaneSize(400)  # the minimum size of a pane.
        self.proportion = proportion
        if not 0 < self.proportion < 1:
            raise ValueError('proportion value for ProportionalSplitter must be between 0 and 1.')
        self.ResetSash()
        self.Bind(wx.EVT_SIZE, self.OnReSize)
        self.Bind(wx.EVT_SPLITTER_SASH_POS_CHANGED, self.OnSashChanged, id=id)
        ##hack to set sizes on first paint event
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.firstpaint = True

    def SplitHorizontally(self, win1, win2):
        if self.GetParent() is None:
            return False
        return wx.SplitterWindow.SplitHorizontally(
            self, win1, win2, int(round(self.GetParent().GetSize().GetHeight() * self.proportion))
        )

    def SplitVertically(self, win1, win2):
        if self.GetParent() is None:
            return False
        return wx.SplitterWindow.SplitVertically(
            self, win1, win2, int(round(self.GetParent().GetSize().GetWidth() * self.proportion))
        )

    def GetExpectedSashPosition(self):
        if self.GetSplitMode() == wx.SPLIT_HORIZONTAL:
            tot = max(self.GetMinimumPaneSize(), self.GetParent().GetClientSize().height)
        else:
            tot = max(self.GetMinimumPaneSize(), self.GetParent().GetClientSize().width)
        return int(round(tot * self.proportion))

    def ResetSash(self):
        self.SetSashPosition(self.GetExpectedSashPosition())

    def OnReSize(self, event):
        'Window has been resized, so we need to adjust the sash based on self.proportion.'
        self.ResetSash()
        event.Skip()

    def OnSashChanged(self, event):
        "We'll change self.proportion now based on where user dragged the sash."
        pos = float(self.GetSashPosition())
        if self.GetSplitMode() == wx.SPLIT_HORIZONTAL:
            tot = max(self.GetMinimumPaneSize(), self.GetParent().GetClientSize().height)
        else:
            tot = max(self.GetMinimumPaneSize(), self.GetParent().GetClientSize().width)
        self.proportion = pos / tot
        event.Skip()

    def OnPaint(self, event):
        if self.firstpaint:
            if self.GetSashPosition() != self.GetExpectedSashPosition():
                self.ResetSash()
            self.firstpaint = False
        event.Skip()


class AlbumDisplay(wx.Panel):
    def __init__(self, *args, **kw):
        super(AlbumDisplay, self).__init__(*args, **kw)

        colsizer = wx.BoxSizer(wx.HORIZONTAL)

        self.art = wx.StaticBitmap(self)
        colsizer.Add(self.art, wx.SizerFlags().Border(wx.TOP | wx.LEFT | wx.RIGHT).Expand())

        self.rhs = wx.Panel(self)
        colsizer.Add(
            self.rhs,
            wx.SizerFlags().Proportion(1).Expand().Border(wx.TOP | wx.LEFT | wx.RIGHT | wx.BOTTOM),
        )

        rhssizer = wx.BoxSizer(wx.VERTICAL)
        self.l_name = wx.StaticText(self.rhs, label='Playlist name')
        rhssizer.Add(self.l_name, wx.SizerFlags().Expand().Border(wx.TOP | wx.LEFT))

        self.tracks = wx.ListCtrl(self.rhs, style=wx.LC_REPORT)
        self.tracks.AppendColumn('Track #', wx.LIST_FORMAT_LEFT, 100)
        self.tracks.AppendColumn('Name', wx.LIST_FORMAT_LEFT, wx.LIST_AUTOSIZE)
        rhssizer.Add(self.tracks, wx.SizerFlags().Proportion(1).Expand().Border(wx.TOP | wx.LEFT))

        self.rhs.SetSizer(rhssizer)
        self.SetSizer(colsizer)

    def update(self, playlist_info):
        self.l_name.SetLabel(playlist_info.title)
        self.art.SetBitmap(wx.BitmapBundle.FromFiles(playlist_info.local_thumb_file_name))
        self.tracks.DeleteAllItems()
        for i, x in enumerate(playlist_info.tracks):
            self.tracks.Append((i + 1, x.title))
        self.tracks.SetColumnWidth(1, wx.LIST_AUTOSIZE)

        widget = self
        while widget.GetParent():
            widget = widget.GetParent()
            widget.Layout()
            if widget.IsTopLevel():
                break

    def get_selected_tracks(self):
        result = []
        item = self.tracks.GetFirstSelected()
        while item != -1:
            result.append(item)
            item = self.tracks.GetNextSelected(item)
        return result

class HelpPane(wx.Panel):
    def __init__(self, playlist_url, *args, **kw):
        super(HelpPane, self).__init__(*args, **kw)

        self.playlist_url = playlist_url

        sizer = wx.BoxSizer(wx.VERTICAL)

        l_help0 = wx.StaticText(self, label='If you have your own playlist handy, paste it above to get started.')
        sizer.Add(l_help0, wx.SizerFlags().DoubleBorder(wx.ALL).Expand())

        l_help1 = wx.StaticText(self, label='Or, try one of these examples:')
        sizer.Add(l_help1, wx.SizerFlags().DoubleBorder(wx.LEFT | wx.TOP).Expand())

        ex0 = wx.adv.HyperlinkCtrl(self, label='Running Tracks', url=r'https://music.youtube.com/playlist?list=RDCLAK5uy_m2DbXGr69B7FSL6LqEHjdEFsxTmghY4Qw')
        ex0.Bind(wx.adv.EVT_HYPERLINK, self.on_click)
        sizer.Add(ex0, wx.SizerFlags().DoubleBorder(wx.LEFT | wx.TOP).Expand())

        ex1 = wx.adv.HyperlinkCtrl(self, label='Marathoners Rocking New York', url=r'https://music.youtube.com/playlist?list=OLAK5uy_l-ZRBmH_xVHk7t1JJgadh41JDyTONLzjE')
        ex1.Bind(wx.adv.EVT_HYPERLINK, self.on_click)
        sizer.Add(ex1, wx.SizerFlags().DoubleBorder(wx.LEFT | wx.TOP).Expand())

        ex2 = wx.adv.HyperlinkCtrl(self, label='Euphoric Running Tracks', url='https://music.youtube.com/playlist?list=RDCLAK5uy_mVuA6l3MZFkCMtL1kWMnvBw6c3YaTCytU')
        ex2.Bind(wx.adv.EVT_HYPERLINK, self.on_click)
        sizer.Add(ex2, wx.SizerFlags().DoubleBorder(wx.LEFT | wx.TOP).Expand())

        ex3 = wx.adv.HyperlinkCtrl(self, label='Rock & Run!', url='https://music.youtube.com/playlist?list=RDCLAK5uy_mx8e76iA69eSkgLpj_oxbrgchR0I5Fa1U')
        ex3.Bind(wx.adv.EVT_HYPERLINK, self.on_click)
        sizer.Add(ex3, wx.SizerFlags().DoubleBorder(wx.LEFT | wx.TOP).Expand())

        self.SetSizer(sizer)

    def on_click(self, event):
        self.playlist_url.SetValue(event.GetURL())


class MainFrame(wx.Frame):
    def __init__(self, *args, **kw):
        # ensure the parent's __init__ is called
        super(MainFrame, self).__init__(*args, **kw)

        self.have_playlist = False
        self.have_watch = False
        self.extra_message = ''
        self.devid_q = None
        self.devindex = -1

        self.proc = None
        self.ensure_daemon_running()

        self.CreateStatusBar(2)
        version = 'Version 0.1β'
        version_size = wx.Window.GetTextExtent(self, version)
        self.SetStatusWidths([-1, version_size.width + 20])
        self.SetStatusText(version, 1)
        self.Bind(wx.EVT_CLOSE, self.on_closed)

        self.splitter = ProportionalSplitter(self)

        lsizer = wx.BoxSizer(wx.VERTICAL)

        lhs = wx.Panel(self.splitter)
        l_playlist = wx.StaticText(lhs, label='Paste YouTube Music playlist or album URL:')
        lsizer.Add(l_playlist, wx.SizerFlags().Border(wx.ALL).Expand())

        self.playlist_url = wx.TextCtrl(lhs)
        lsizer.Add(self.playlist_url, wx.SizerFlags().Border(wx.LEFT | wx.RIGHT).Expand())
        self.playlist_url.Bind(wx.EVT_SET_FOCUS, self.highlight_url)
        self.playlist_url.Bind(wx.EVT_TEXT, self.refresh_playlist)

        self.help = HelpPane(parent=lhs, playlist_url=self.playlist_url)
        lsizer.Add(self.help, wx.SizerFlags().Proportion(1).Expand())

        self.album = AlbumDisplay(lhs)
        lsizer.Add(self.album, wx.SizerFlags().Proportion(1).Expand())

        pair = wx.Panel(lhs)
        pairsizer = wx.BoxSizer(wx.HORIZONTAL)

        self.go_sel = wx.Button(pair)
        self.go_sel.SetLabel('Copy selected tracks to watch ' + '➤')
        self.go_sel.Bind(wx.EVT_BUTTON, self.on_sync_selected)
        pairsizer.Add(self.go_sel, wx.SizerFlags().Border(wx.ALL).ReserveSpaceEvenIfHidden())
        self.go_sel.Hide()

        self.go_all = wx.Button(pair)
        self.go_all.SetLabel('Copy whole playlist to watch ' + '➤➤➤')
        self.go_all.Bind(wx.EVT_BUTTON, self.on_sync_all)
        pairsizer.Add(self.go_all, wx.SizerFlags().Border(wx.ALL).ReserveSpaceEvenIfHidden())
        self.go_all.Hide()

        pair.SetSizer(pairsizer)
        lsizer.Add(pair, wx.SizerFlags().Align(wx.ALIGN_RIGHT))

        rhs = wx.Panel(self.splitter)
        rsizer = wx.BoxSizer(wx.VERTICAL)

        l_watch = wx.StaticText(rhs, label='Watch to copy to:')
        rsizer.Add(l_watch, wx.SizerFlags().Border(wx.TOP | wx.LEFT))

        self.devices = wx.Choice(rhs)
        self.devices.Bind(wx.EVT_CHOICE, self.on_device_selected)
        rsizer.Add(self.devices, wx.SizerFlags().Border(wx.ALL).Expand())

        self.l_currentmusic = wx.StaticText(rhs, label='Current music:')
        rsizer.Add(self.l_currentmusic, wx.SizerFlags().Border(wx.TOP | wx.LEFT))

        self.watch_song_data = []
        self.music_objectid = None
        self.lc_watch_contents = wx.ListCtrl(rhs, style=wx.LC_REPORT)
        self.lc_watch_contents.AppendColumn('File name', wx.LIST_FORMAT_LEFT, wx.LIST_AUTOSIZE)
        rsizer.Add(
            self.lc_watch_contents,
            wx.SizerFlags().Proportion(1).Expand().Border(wx.TOP | wx.LEFT | wx.RIGHT),
        )

        self.delete_sel = wx.Button(rhs)
        self.delete_sel.SetLabel('Delete selected files from watch ' + '❌')
        self.delete_sel.Bind(wx.EVT_BUTTON, self.on_delete_selected)
        rsizer.Add(self.delete_sel, wx.SizerFlags().Align(wx.ALIGN_RIGHT).Border(wx.ALL))

        lhs.SetSizer(lsizer)
        rhs.SetSizer(rsizer)

        self.splitter.SplitVertically(lhs, rhs)

        self.update_device_list()

    def update_status(self, state_enum, status, message=None):
        if state_enum == STATE_PLAYLIST_LOADED:
            self.have_playlist = status
            self.extra_message = message

        if state_enum == STATE_WATCH_CONNECTED:
            self.have_watch = status

        if not self.have_playlist:
            display = 'Enter a playlist or album URL above.'
        elif self.have_playlist and not self.have_watch:
            display = 'Connect your watch via USB.'
        else:
            display = 'Use the "transfer" buttons to send music to your watch.'

        if not self.have_playlist:
            self.album.Hide()
            self.help.Show()
        else:
            self.help.Hide()
            self.album.Show()

        if not self.have_watch:
            self.l_currentmusic.Hide()
            self.lc_watch_contents.Hide()
            self.delete_sel.Hide()
        else:
            self.l_currentmusic.Show()
            self.lc_watch_contents.Show()
            self.delete_sel.Show()

        if not self.have_playlist or not self.have_watch:
            self.go_sel.Hide()
            self.go_all.Hide()
        else:
            self.go_sel.Show()
            self.go_all.Show()

        if self.extra_message:
            display = display + ' -- ' + self.extra_message

        self.SetStatusText(display, 0)

    def highlight_url(self, event):
        def delayed(ev):
            self.Unbind(wx.EVT_IDLE)
            self.playlist_url.SelectAll()

        self.Bind(wx.EVT_IDLE, delayed)
        event.Skip()

    def refresh_playlist(self, event):
        event.Skip()

        def refresh():
            error_message = None
            try:
                url = self.playlist_url.GetValue().strip()
                if not url:
                    self.current_playlist = None
                else:
                    self.current_playlist = localcache.PlaylistInfo.create(url)
            except yt_dlp.utils.DownloadError as err:
                error_message = err.msg

            def update_ui():
                if error_message or not self.current_playlist:
                    self.update_status(STATE_PLAYLIST_LOADED, False, error_message)
                else:
                    self.update_status(STATE_PLAYLIST_LOADED, True)
                    # Stupid order is important. The listctrl has to relayout
                    # itself here after it's visible
                    self.album.update(self.current_playlist)

            wx.CallAfter(update_ui)

        th = threading.Thread(target=refresh)
        th.daemon = True
        th.start()

    def ensure_daemon_running(self):
        if not self.proc or self.proc.poll() is not None:
            torun = os.path.join(SELFDIR, 'bin', 'wpd-mtp-helper.exe')
            logpipe = LogPipe(logging.INFO)
            self.proc = subprocess.Popen(
                [torun, '--daemon'], stdout=subprocess.PIPE, stderr=logpipe
            )
            logpipe.close()
            startup_data = json.loads(self.proc.stdout.readline())
            self.base_url = 'http://127.0.0.1:%d/' % startup_data['port']

    def daemon_request(self, target, callback, method='GET'):
        self.ensure_daemon_running()
        # logging.debug('REQUEST: %s' % (self.base_url + target))
        req = urllib.request.Request(url=self.base_url + target, method=method)
        resp = json.loads(urllib.request.urlopen(req).read())
        # logging.debug('RESPONSE: %s' % resp)
        wx.CallAfter(callback, resp)

    def on_closed(self, event):
        self.proc.terminate()
        self.Destroy()

    def update_device_list(self):
        def update():
            self.daemon_request('devices', self.update_retrieved_list)
            time.sleep(1)
            self.update_device_list()

        th = threading.Thread(target=update)
        th.daemon = True
        th.start()

    def format_user_label(self, d):
        return '%s (%s)' % (d['friendly'], d['manufacturer'])

    def update_retrieved_list(self, data):
        # logging.debug(data)
        keep = [x for x in data if x['manufacturer'] == 'Garmin']
        do_refresh = len(keep) == 0
        if len(keep) == self.devices.GetCount():
            current_items = []
            for i in range(self.devices.GetCount()):
                current_items.append(self.devices.GetString(i))
            for k, c in zip(keep, current_items):
                if self.format_user_label(k) != c:
                    do_refresh = True
        else:
            do_refresh = True

        if do_refresh:
            self.device_data = []
            self.devindex = -1
            self.devid_q = None
            self.watch_song_data = []
            self.music_objectid = None
            self.lc_watch_contents.DeleteAllItems()

            while self.devices.GetCount():
                self.devices.Delete(0)

            if len(keep) == 0:
                self.devices.SetItems(
                    [
                        'Waiting for watch USB connection... '
                        '(can take up to a minute after plugging in!)'
                    ]
                )
                self.devices.SetSelection(0)
                self.update_status(STATE_WATCH_CONNECTED, False)
            else:
                self.device_data = keep
                self.devices.SetItems([self.format_user_label(x) for x in keep])
                self.devices.SetSelection(0)
                self.select_device(0)
                self.update_status(STATE_WATCH_CONNECTED, True)

    def on_device_selected(self, event):
        index = event.GetInt()
        if index < len(self.device_data):
            self.select_device(index)

    def refresh_device_contents(self):
        self.select_device(self.devindex)  # Causes reload of contents

    def select_device(self, index):
        def bulk_get():
            self.devindex = index
            self.devid_q = urllib.parse.quote(self.device_data[index]['id'])
            self.daemon_request('bulk-properties/%s' % self.devid_q, self.update_current_music_list)

        self.lc_watch_contents.DeleteAllItems()
        self.lc_watch_contents.Append(('Loading...',))
        self.lc_watch_contents.SetColumnWidth(0, 200)
        self.lc_watch_contents.Disable()

        th = threading.Thread(target=bulk_get)
        th.daemon = True
        th.start()

    def find_single_node(self, data, parent, content_type, name=None):
        for n in data:
            if n['WPD_OBJECT_PARENT_ID'] == parent and n['WPD_OBJECT_CONTENT_TYPE'] == content_type:
                if name is None or n.get('WPD_OBJECT_NAME') == name:
                    return n
        raise NameError('parent=%s, content_type=%s, name=%s' % (parent, content_type, name))

    def find_all_nodes(self, data, parent, content_type):
        result = []
        for n in data:
            if n['WPD_OBJECT_PARENT_ID'] == parent and n['WPD_OBJECT_CONTENT_TYPE'] == content_type:
                result.append(n)
        return result

    def update_current_music_list(self, data):
        """
        WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT = root, PARENT_ID = ""
        WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT = Media, PARENT_ID = DEVICE
        WPD_CONTENT_TYPE_FOLDER = Music, PARENT_ID = s20002 = SID_20002, mapping?
        WPD_CONTENT_TYPE_AUDIO = blah.mp3, PARENT_ID = id of Music
        """
        root = self.find_single_node(
            data, parent='', content_type=WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT
        )
        media = self.find_single_node(
            data,
            parent=root['WPD_OBJECT_ID'],
            content_type=WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT,
            name='Media',
        )
        music = self.find_single_node(
            data, parent=media['WPD_OBJECT_ID'], content_type=WPD_CONTENT_TYPE_FOLDER, name='Music'
        )
        songs = self.find_all_nodes(
            data, parent=music['WPD_OBJECT_ID'], content_type=WPD_CONTENT_TYPE_AUDIO
        )
        self.music_objectid = music['WPD_OBJECT_ID']
        self.watch_song_data = songs
        self.lc_watch_contents.DeleteAllItems()
        for x in songs:
            self.lc_watch_contents.Append((x.get('WPD_OBJECT_ORIGINAL_FILE_NAME'),))
        self.lc_watch_contents.SetColumnWidth(0, wx.LIST_AUTOSIZE)
        self.lc_watch_contents.Enable()

    def cache_tracks(self, tracks, rest):
        self.cache_dialog = wx.ProgressDialog(
            'Downloading from YouTube...',
            'Retrieving music from YouTube...',
            len(tracks),
            parent=self,
            style=wx.PD_APP_MODAL | wx.PD_AUTO_HIDE | wx.PD_CAN_ABORT,
        )

        def cache():
            want_abort = False
            for i, t in enumerate(tracks):
                if want_abort:
                    break

                def upd(index, title):
                    nonlocal want_abort
                    ret = self.cache_dialog.Update(index, title)
                    if ret[0] == False:
                        want_abort = True

                t.ensure_cached()
                wx.CallAfter(upd, i + 1, t.title)

            wx.CallAfter(lambda: self.cache_dialog.Update(len(tracks)))
            wx.CallAfter(lambda: self.cache_dialog.Destroy())

            if not want_abort:
                wx.CallAfter(rest, tracks)

        th = threading.Thread(target=cache)
        th.daemon = True
        th.start()

    def do_send(self, tracks):
        self.send_dialog = wx.ProgressDialog(
            'Uploading to watch...',
            'Copying music to device...',
            len(tracks),
            parent=self,
            style=wx.PD_APP_MODAL | wx.PD_AUTO_HIDE | wx.PD_CAN_ABORT,
        )

        def safer_name(name):
            return re.sub(r'[\\/:"*?<>|]+', '', name)

        def send():
            if not self.music_objectid:
                logging.error('no music object, aborting')
                return

            want_abort = False
            for i, t in enumerate(tracks):
                if want_abort:
                    break

                def upd(index, title):
                    nonlocal want_abort
                    ret = self.send_dialog.Update(index, title)
                    if ret[0] == False:
                        want_abort = True

                if os.path.isfile(t.local_file_name):
                    objid_q = urllib.parse.quote(self.music_objectid)
                    local_q = urllib.parse.quote(t.local_file_name)
                    title_q = urllib.parse.quote(safer_name(t.title) + '.mp3')
                    self.daemon_request(
                        'send-file/%s?parentid=%s&localfilename=%s&targettitle=%s'
                        % (self.devid_q, objid_q, local_q, title_q),
                        lambda r: True,
                        method='POST',
                    )
                    wx.CallAfter(upd, i + 1, t.title)
                else:
                    want_abort = True

            wx.CallAfter(lambda: self.send_dialog.Update(len(tracks)))
            wx.CallAfter(lambda: self.send_dialog.Destroy())

            if not want_abort:
                wx.CallAfter(self.refresh_device_contents)

        th = threading.Thread(target=send)
        th.daemon = True
        th.start()

    def on_sync_selected(self, event):
        track_indices = self.album.get_selected_tracks()
        if track_indices:
            self.cache_tracks(
                [self.current_playlist.tracks[i] for i in track_indices], self.do_send
            )

    def on_sync_all(self, event):
        self.cache_tracks(self.current_playlist.tracks, self.do_send)

    def on_delete_selected(self, event):
        if not self.devid_q or self.devindex < 0:
            return

        sel = []
        i = self.lc_watch_contents.GetFirstSelected()
        while i != -1:
            sel.append(self.watch_song_data[i]['WPD_OBJECT_ID'])
            i = self.lc_watch_contents.GetNextSelected(i)

        def do_deletes():
            count = [0]

            def refresh_on_last(resp):
                count[0] += 1
                if count[0] == len(sel):
                    self.refresh_device_contents()

            for obj in sel:
                quotedobj = urllib.parse.quote(obj)
                self.daemon_request(
                    'delete-object/%s?objectid=%s' % (self.devid_q, quotedobj),
                    refresh_on_last,
                    method='POST',
                )

        th = threading.Thread(target=do_deletes)
        th.daemon = True
        th.start()


if __name__ == '__main__':
    # When this module is run (not imported) then create the app, the
    # frame, show it, and start the event loop.
    app = wx.App()
    frm = MainFrame(None, title='Jog Jams')
    frm.SetSize(frm.FromDIP(wx.Size(1200, 800)))
    frm.SetMinSize(frm.FromDIP(wx.Size(800, 600)))
    frm.SetIcon(wx.Icon(os.path.join(SELFDIR, 'logo2-32.png')))
    frm.Show()
    app.MainLoop()
