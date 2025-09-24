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
import json

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

class Account():
    def __init__(self, name, signature):
        self.name = name
        self.signature = signature


home_dir = str(Path.home())
mount_dir = home_dir + '/SeaDrive'

seafile_pipe_path = home_dir + '/.seadrive/seadrive_ext.sock'

class SeaDriveFileExtension(GObject.GObject, Nautilus.MenuProvider, Nautilus.InfoProvider):

    def __init__(self):
        super().__init__()
        self.conn = GuiConnection(seafile_pipe_path)

    def get_file_items(self, *args):
        files = args[-1]
        if files == None or len(files) != 1:
            return None

        file = files[0]

        file_path = self.uri_to_local_path(file.get_uri())
        if not file_path.startswith(mount_dir) or file_path == mount_dir:
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
                label="Get Upload Link",
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

        try:
            is_cached = self.conn.send("is-file-cached", file_path)
        except Exception as e:
            print(f"Failed to get file cached state for {file_path}: {e}")
            return None

        submenu = Nautilus.Menu()

        if status == "locked_by_me":
            unlock_item = Nautilus.MenuItem(
                name="SeaDriveExt::UnlockFile",
                label="Unlock File",
                tip="Click to unlock file"
            )
            unlock_item.connect('activate', self.on_unlock_file, file_path, file)
            submenu.append_item(unlock_item)
        elif status  != "locked":
            lock_item = Nautilus.MenuItem(
                name="SeaDriveExt::LockFile",
                label="Lock File",
                tip="Click to lock file"
            )
            lock_item.connect('activate', self.on_lock_file, file_path, file)
            submenu.append_item(lock_item)

        if is_cached == "cached":
            uncache_item = Nautilus.MenuItem(
                name="SeaDriveExt::UnCacheFile",
                label="Remove cache",
                tip="Click to remove file cache"
            )
            uncache_item.connect('activate', self.on_uncache_file, file_path, file)
            submenu.append_item(uncache_item)
        else: 
            cache_item = Nautilus.MenuItem(
                name="SeaDriveExt::CacheFile",
                label="Download",
                tip="Click to download file"
            )
            cache_item.connect('activate', self.on_cache_file, file_path, file)
            submenu.append_item(cache_item)

        link_item = Nautilus.MenuItem(
            name="SeaDriveExt::ShareLink",
            label="Get Share Link",
            tip="Click to get share link"
        )
        link_item.connect('activate', self.on_get_share_link, file_path)
        submenu.append_item(link_item)

        internal_item = Nautilus.MenuItem(
            name="SeaDriveExt::InernalLink",
            label="Get Internal Link",
            tip="Click to get internal link"
        )
        internal_item.connect('activate', self.on_get_internal_link, file_path)
        submenu.append_item(internal_item)

        history_item = Nautilus.MenuItem(
            name="SeaDriveExt::FileHistory",
            label="Show File History",
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
            # Refresh Nautilus by setting extended attributes.
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

    def on_cache_file(self, menu_item, file_path, file):
        try:
            self.conn.send("download", file_path)
        except Exception as e:
            print(f"Failed to download file {file_path}: {e}")

    def on_uncache_file(self, menu_item, file_path, file):
        try:
            self.conn.send("uncache", file_path)
            # Set a temporary extended attribute to notify Nautilus to update the file status.
            os.setxattr(file_path, "user.update-file-info", b"true")
        except Exception as e:
            print(f"Failed to uncache file {file_path}: {e}")

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

    def get_background_items(self, *args):
        folder = args[-1]
        folder_path = self.uri_to_local_path(folder.get_uri())
        if folder_path != mount_dir:
            return None
        seadrive_menu_item = Nautilus.MenuItem(
            name="SeaDriveExt::SeaDrive",
            label="SeaDrive",
            tip="SeaDrive menus"
        )
        seadrive_menu = Nautilus.Menu()

        progress_item = Nautilus.MenuItem(
            name="SeaDriveExt::TransferProgress",
            label="Transfer Progress",
            tip="Click to get trnasfer progress"
        )
        progress_item.connect('activate', self.on_transfer_progress)
        seadrive_menu.append_item(progress_item)

        sync_errors_item = Nautilus.MenuItem(
            name="SeaDriveExt::FileSyncErrors",
            label="Show File Sync Errors",
            tip="Click to show file sync errors"
        )
        sync_errors_item.connect('activate', self.on_show_file_sync_errors)
        seadrive_menu.append_item(sync_errors_item)

        enc_libs_item = Nautilus.MenuItem(
            name="SeaDriveExt::EncLibs",
            label="Show Encrypted Libraries",
            tip="Click to show encrypted libraries"
        )
        enc_libs_item.connect('activate', self.on_show_enc_libs)
        seadrive_menu.append_item(enc_libs_item)

        open_logs_folder_item = Nautilus.MenuItem(
            name="SeaDriveExt::OpenLogsFolder",
            label="Open Logs Folder",
            tip="Click to open logs folder"
        )
        open_logs_folder_item.connect('activate', self.on_open_logs_folder)
        seadrive_menu.append_item(open_logs_folder_item)

        settings_item = Nautilus.MenuItem(
            name="SeaDriveExt::ShowSettings",
            label="Settings",
            tip="Click to open settings"
        )
        settings_item.connect('activate', self.on_show_settings)
        seadrive_menu.append_item(settings_item)

        accounts = []
        try:
            rsp = self.conn.send("show-accounts", "")
            if rsp != "":
                data = json.loads(rsp)
                for account in data:
                    accounts.append(Account(account["name"], account["signature"]))
        except Exception as e:
            print(f"Failed to get accounts: {e}")

        accounts_menu_item = Nautilus.MenuItem(
            name="SeaDriveExt::Accounts",
            label="Accounts",
            tip="Accounts menus"
        )
        accounts_menu = Nautilus.Menu()
        for account in accounts:
            account_menu_item = Nautilus.MenuItem(
                name="SeaDriveExt::Account",
                label=account.name,
                tip="Account menus"
            )
            accounts_menu.append_item(account_menu_item)

            account_menu = Nautilus.Menu()
            resync_account_item = Nautilus.MenuItem(
                name="SeaDriveExt::ResyncAccount",
                label="Resync",
                tip="Click to resync account"
            )
            resync_account_item.connect('activate', self.on_resync_account, account)
            account_menu.append_item(resync_account_item)

            delete_account_item = Nautilus.MenuItem(
                name="SeaDriveExt::DeleteAccount",
                label="Delete",
                tip="Click to delete account"
            )
            delete_account_item.connect('activate', self.on_delete_account, account)
            account_menu.append_item(delete_account_item)

            account_menu_item.set_submenu(account_menu)

        add_account_item = Nautilus.MenuItem(
            name="SeaDriveExt::AddAccount",
            label="Add an account",
            tip="Click to add an account"
        )
        add_account_item.connect('activate', self.on_add_account)

        #accounts_menu.append_item(add_account_item)
        #accounts_item.set_submenu(accounts_menu)
        accounts_menu.append_item(add_account_item)
        accounts_menu_item.set_submenu(accounts_menu)
        seadrive_menu.append_item(accounts_menu_item)

        about_item = Nautilus.MenuItem(
            name="SeaDriveExt::ShowAbout",
            label="About",
            tip="Click to open about"
        )
        about_item.connect('activate', self.on_show_about)
        seadrive_menu.append_item(about_item)

        help_item = Nautilus.MenuItem(
            name="SeaDriveExt::ShowHelp",
            label="Online Help",
            tip="Click to open online help"
        )
        help_item.connect('activate', self.on_show_help)
        seadrive_menu.append_item(help_item)

        quit_item = Nautilus.MenuItem(
            name="SeaDriveExt::Quit",
            label="Quit",
            tip="Click to quit seadrive"
        )
        quit_item.connect('activate', self.on_quit)
        seadrive_menu.append_item(quit_item)

        seadrive_menu_item.set_submenu(seadrive_menu)

        return [seadrive_menu_item]
    
    def on_transfer_progress(self, menu_item):
        try:
            self.conn.send("show-transfer-progress", "")
        except Exception as e:
            print(f"Failed to get transfer progress: {e}")
        return

    def on_show_file_sync_errors(self, menu_item):
        try:
            self.conn.send("show-file-sync-errors", "")
        except Exception as e:
            print(f"Failed to show file sync errors: {e}")
        return

    def on_show_enc_libs(self, menu_item):
        try:
            self.conn.send("show-encrypted-libraries", "")
        except Exception as e:
            print(f"Failed to show encrypted libraries: {e}")
        return

    def on_open_logs_folder(self, menu_item):
        try:
            self.conn.send("open-logs-folder", "")
        except Exception as e:
            print(f"Failed to open logs folder: {e}")
        return

    def on_show_settings (self, menu_item):
        try:
            self.conn.send("show-settings", "")
        except Exception as e:
            print(f"Failed to show settings: {e}")
        return

    def on_add_account (self, menu_item):
        try:
            self.conn.send("add-account", "")
        except Exception as e:
            print(f"Failed to add account: {e}")
        return

    def on_resync_account (self, menu_item, account):
        try:
            self.conn.send("resync-account", account.signature)
            print(account.name, account.signature)
        except Exception as e:
            print(f"Failed to resync account: {e}")
        return

    def on_delete_account (self, menu_item, account):
        try:
            self.conn.send("delete-account", account.signature)
        except Exception as e:
            print(f"Failed to delete account: {e}")
        return

    def on_show_about(self, menu_item):
        try:
            self.conn.send("show-about", "")
        except Exception as e:
            print(f"Failed to show about: {e}")
        return

    def on_show_help(self, menu_item):
        try:
            self.conn.send("show-help", "")
        except Exception as e:
            print(f"Failed to show online help: {e}")
        return

    def on_quit(self, menu_item):
        try:
            self.conn.send("quit", "")
        except Exception as e:
            print(f"Failed to quit seadrive: {e}")
        return
