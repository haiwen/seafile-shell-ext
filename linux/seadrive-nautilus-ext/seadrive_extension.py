import os
import socket
import queue
import struct
import subprocess
from gi.repository import Nautilus, GObject
from pathlib import Path
import urllib.parse

import os
import socket

LOCK_TAG="lock-file"

class Transport():
    def __init__(self, pipe_path):
        self.pipe_path = pipe_path
        self.conn = None

    def connect(self):
        self.conn = socket.socket(socket.AF_UNIX)
        self.conn.connect(self.pipe_path)

    def stop(self):
        if self.conn:
            self.conn.close()
            self.conn= None

    def send(self, name, path):
        body = name + "\t" + path
        body_utf8 = body.encode(encoding='utf-8')
        # "I" for unsiged int
        header = struct.pack('=I', len(body_utf8))
        self.sendall(header)
        self.sendall(body_utf8)

        resp_header = self.recvall(4)
        resp_size, = struct.unpack('=I', resp_header)
        resp = self.recvall(resp_size)
        return resp.decode(encoding='utf-8')

    def recvall(self, total):
        remain = total
        data = bytearray()
        while remain > 0:
            new = self.conn.recv(remain)

            n = len(new)
            if n <= 0:
                raise RuntimeError("Failed to read from socket")
            else:
                data.extend(new)
                remain -= n

        return bytes(data)

    def sendall(self, data):
        total = len(data)
        offset = 0
        while offset < total:
            n = self.conn.send(data[offset:])

            if n <= 0:
                raise RuntimeError("Failed to write to socket")
            else:
                offset += n

class GuiConnection():
    def __init__(self, pipe_path, pool_size=5):
        self.pipe_path = pipe_path
        self.pool_size = pool_size
        self._pool = queue.Queue(pool_size)

    def _create_transport(self):
        transport = Transport(self.pipe_path)
        transport.connect()
        return transport

    def _get_transport(self):
        try:
            transport = self._pool.get(False)
        except:
            transport = self._create_transport()
        return transport

    def _return_transport(self, transport):
        try:
            self._pool.put(transport, False)
        except queue.Full:
            transport.stop()

    def send(self, name, path):
        transport = self._get_transport()
        ret_str = transport.send(name, path)
        self._return_transport(transport)
        return ret_str

home_dir = str(Path.home())
mount_dir = home_dir + '/SeaDrive'

seafile_pipe_path = home_dir + '/.seadrive/seadrive_ext.sock'

class SeaDriveFileExtension(GObject.GObject, Nautilus.MenuProvider, Nautilus.InfoProvider):

    def __init__(self):
        super().__init__()
        self.conn = GuiConnection(seafile_pipe_path)

    def get_file_items(self, window, files):
        if files == None or len(files) != 1:
            return None

        file = files[0]

        file_path = self.uri_to_local_path(file.get_uri())
        if not file_path.startswith(mount_dir):
            return None

        parent_menu = Nautilus.MenuItem(
            name="SeaDriveExt::SeaDrive",
            label="SeaDrive",
            tip="SeaDrive menus"
        )

        try:
            is_in_repo = self.conn.send("is-file-in-repo", file_path)
            if is_in_repo != "true":
                return None
        except Exception as e:
            return None

        if file.is_directory():
            upload_item = Nautilus.MenuItem(
                name="SeaDriveExt::UploadLink",
                label="get upload link",
                tip="Click to get upload link"
            )
            upload_item.connect('activate', self.on_get_upload_link, file_path)

            submenu = Nautilus.Menu()
            submenu.append_item(upload_item)

            parent_menu.set_submenu(submenu)

            return [parent_menu]

        try:
            status = self.conn.send("get-file-status", file_path)
        except Exception as e:
            print(f"Failed to get file lock state for {file_path}: {e}")
            return None

        submenu = Nautilus.Menu()

        if status == "locked_by_me":
            unlock_item = Nautilus.MenuItem(
                name="SeaDriveExt::UnlockFile",
                label="unlock file",
                tip="Click to unlock file"
            )
            unlock_item.connect('activate', self.on_unlock_file, file_path, file)
            submenu.append_item(unlock_item)
        elif status  != "locked":
            lock_item = Nautilus.MenuItem(
                name="SeaDriveExt::LockFile",
                label="lock file",
                tip="Click to lock file"
            )
            lock_item.connect('activate', self.on_lock_file, file_path, file)
            submenu.append_item(lock_item)

        link_item = Nautilus.MenuItem(
            name="SeaDriveExt::ShareLink",
            label="get share link",
            tip="Click to get share link"
        )
        link_item.connect('activate', self.on_get_share_link, file_path)
        submenu.append_item(link_item)

        internal_item = Nautilus.MenuItem(
            name="SeaDriveExt::InernalLink",
            label="get internal link",
            tip="Click to get internal link"
        )
        internal_item.connect('activate', self.on_get_internal_link, file_path)
        submenu.append_item(internal_item)

        history_item = Nautilus.MenuItem(
            name="SeaDriveExt::FileHistory",
            label="show file history",
            tip="Click to show file history"
        )
        history_item.connect('activate', self.on_show_file_history, file_path)
        submenu.append_item(history_item)

        parent_menu.set_submenu(submenu)

        return [parent_menu]

    def on_get_upload_link(self, menu_item, file_path):
        try:
            self.conn.send("get-upload-link", file_path)
        except Exception as e:
            print(f"Failed to get upload link for {file_path}: {e}")

    def on_get_share_link(self, menu_item, file_path):
        try:
            self.conn.send("get-share-link", file_path)
        except Exception as e:
            print(f"Failed to get share link for {file_path}: {e}")

    def on_get_internal_link(self, menu_item, file_path):
        try:
            self.conn.send("get-internal-link", file_path)
        except Exception as e:
            print(f"Failed to get internal link for {file_path}: {e}")

    def on_show_file_history (self, menu_item, file_path):
        try:
            self.conn.send("show-history", file_path)
        except Exception as e:
            print(f"Failed to show file history for {file_path}: {e}")

    def on_unlock_file(self, menu_item, file_path, file):
        try:
            self.conn.send("unlock-file", file_path)
            os.setxattr(file_path, "user.seafile-status", b"unlocked")
        except Exception as e:
            print(f"Failed to unlock file {file_path}: {e}")

    def on_lock_file(self, menu_item, file_path, file):
        try:
            self.conn.send("lock-file", file_path)
            # Refresh Nautilus by setting extended attributes.
            os.setxattr(file_path, "user.seafile-status", b"locked-by-me")

        except Exception as e:
            print(f"Failed to lock file {file_path}: {e}")

    def uri_to_local_path(self, uri):
        if uri.startswith('file://'):
            path = Path(urllib.parse.unquote(uri[7:])).resolve()
            return str(path)
        path = Path(uri).resolve()
        return str(path)

    def update_file_info(self, file_info):
        if file_info.is_directory():
            return
        file_path = self.uri_to_local_path(file_info.get_uri())
        if not file_path.startswith(mount_dir):
            return
        try:
            status = self.conn.send("get-file-status", file_path)
            if status == "locked":
                file_info.add_emblem("emblem-seadrive-locked-by-others")
            elif status == "locked_by_me":
                file_info.add_emblem("emblem-seadrive-locked-by-me")
            elif status == "syncing":
                file_info.add_emblem("emblem-seadrive-syncing")
            elif status == "synced":
                file_info.add_emblem("emblem-seadrive-done")
        except Exception as e:
            print(f"Failed to get file status for {file_path}: {e}")
        return
