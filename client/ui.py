import os, sys
from ftp import FtpClient
from shutil import rmtree
from PyQt5.QtWidgets import (QApplication, QWidget, QLineEdit,
                             QVBoxLayout, QHBoxLayout, QLabel,
                             QPushButton, QTextEdit, QTreeWidget,
                             QCompleter, QTreeWidgetItem, QInputDialog,
                             QComboBox, QDialog, QRadioButton,
                             QButtonGroup, QDialogButtonBox)
from PyQt5.QtGui import QRegExpValidator, QIcon, QColor
from PyQt5.QtCore import *


class MyThread(QThread):
    def __init__(self, f, finish=None):
        super().__init__()
        self.f = f
        if finish:
            self.onfinish = finish
            self.finished.connect(self.onfinish)

    def run(self):
        self.f()


class RadioDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)

        buttons = [QRadioButton('overwrite'), QRadioButton('append'), QRadioButton('rest')]
        self.button_group = QButtonGroup(self)
        for index, button in enumerate(buttons):
            layout.addWidget(button)
            self.button_group.addButton(button, index)

        self.bottom_buttons = QDialogButtonBox(
            QDialogButtonBox.Ok | QDialogButtonBox.Cancel,
            Qt.Horizontal, self)
        self.bottom_buttons.button(QDialogButtonBox.Ok).setEnabled(False)
        self.bottom_buttons.accepted.connect(self.accept)
        self.bottom_buttons.rejected.connect(self.reject)
        layout.addWidget(self.bottom_buttons)

        self.button_group.buttonClicked.connect(self.enable_button)

    def enable_button(self):
        self.bottom_buttons.button(QDialogButtonBox.Ok).setEnabled(True)

    def text(self):
        return self.button_group.checkedButton().text()

    @staticmethod
    def getOption(parent=None):
        dialog = RadioDialog(parent)
        result = dialog.exec_()
        text = dialog.text()
        return text, result == QDialog.Accepted


class FilelistWidget(QWidget):

    def __init__(self, text='', parent=None):
        super().__init__(parent)
        self.main_layout = QVBoxLayout(self)

        self.init_filelist()
        self.init_buttons()

        self.label = QLabel(text, self)
        self.path_edit = QLineEdit(self)
        completer = QCompleter()
        self.completer_model = QStringListModel()
        completer.setModel(self.completer_model)
        self.path_edit.setCompleter(completer)
        self.button_layout = QHBoxLayout(self)
        self.button_layout.addWidget(self.download_btn)
        self.button_layout.addWidget(self.delete_btn)
        self.button_layout.addWidget(self.rename_btn)
        self.button_layout.addWidget(self.mkdir_btn)

        self.main_layout.addWidget(self.label)
        self.main_layout.addWidget(self.path_edit)
        self.main_layout.addWidget(self.tree)
        self.main_layout.addLayout(self.button_layout)

        self.setLayout(self.main_layout)

    def init_filelist(self):
        self.tree = QTreeWidget(self)
        self.tree.setIconSize(QSize(20, 20))
        self.tree.setRootIsDecorated(False)
        self.tree.setHeaderLabels(('Name', 'Size', 'Time', 'Owner', 'Group'))

    def init_buttons(self):
        self.download_btn = QPushButton('Download', self)
        self.delete_btn = QPushButton('Delete', self)
        self.rename_btn = QPushButton('Rename', self)
        self.mkdir_btn = QPushButton('New Dir', self)


class App(QWidget):

    local_path = os.getenv('HOME')
    local_complete_list = []
    remote_path = ''
    remote_complete_list = []

    fileicon_path = 'img/file.png'
    foldericon_path = 'img/folder.png'

    connected = False

    def __init__(self):
        super().__init__()
        self.ftp = FtpClient()
        self.title = 'Simple FTP client'
        self.width = 800
        self.height = 800
        self.init_ui()
        self.validate_input()
        self.get_local_filelist()

        self.connbtn.clicked.connect(self.connect)
        self.local.tree.itemDoubleClicked.connect(self.local_change_dir_item)
        self.local.path_edit.returnPressed.connect(self.local_change_dir_byedit)
        self.local.download_btn.clicked.connect(self.local_upload_selected_item)
        self.local.delete_btn.clicked.connect(self.local_delete_file)
        self.local.rename_btn.clicked.connect(self.local_rename_file)
        self.local.mkdir_btn.clicked.connect(self.local_mkdir)
        self.remote.tree.itemDoubleClicked.connect(self.remote_change_dir_item)
        self.remote.path_edit.returnPressed.connect(self.remote_change_dir_byedit)
        self.remote.download_btn.clicked.connect(self.remote_download_selected_item)
        self.remote.delete_btn.clicked.connect(self.remote_delete_file)
        self.remote.rename_btn.clicked.connect(self.remote_rename_file)
        self.remote.mkdir_btn.clicked.connect(self.remote_mkdir)

        self.mode_select.currentIndexChanged.connect(self.set_mode)

    def init_ui(self):
        self.setWindowTitle(self.title)
        self.setFixedSize(self.width, self.height)
        total = QVBoxLayout()
        self.setLayout(total)

        edits = QHBoxLayout()
        self.hostlabel = QLabel(self)
        self.hostlabel.setText('Host:')
        self.hostbox = QLineEdit(self)
        self.userlabel = QLabel(self)
        self.userlabel.setText('Username:')
        self.userbox = QLineEdit(self)
        self.paswlabel = QLabel(self)
        self.paswlabel.setText('Password')
        self.paswbox = QLineEdit(self)
        self.paswbox.setEchoMode(QLineEdit.Password)
        self.portlabel = QLabel(self)
        self.portlabel.setText('Port:')
        self.portbox = QLineEdit(self)
        self.mode_select = QComboBox(self)
        self.mode_select.addItems(['PASV', 'PORT'])
        self.connbtn = QPushButton('Connect', self)

        edits.addWidget(self.hostlabel)
        edits.addWidget(self.hostbox)
        edits.addWidget(self.userlabel)
        edits.addWidget(self.userbox)
        edits.addWidget(self.paswlabel)
        edits.addWidget(self.paswbox)
        edits.addWidget(self.portlabel)
        edits.addWidget(self.portbox)
        edits.addWidget(self.mode_select)
        edits.addWidget(self.connbtn)
        total.addLayout(edits)

        self.messagebox = QTextEdit(self)
        self.messagebox.setReadOnly(True)
        self.messagebox.setMaximumHeight(200)
        total.addWidget(self.messagebox)

        self.local= FilelistWidget('Local', self)
        self.local.download_btn.setText('Upload')
        self.remote = FilelistWidget('Remote', self)
        both = QHBoxLayout(self)
        both.addWidget(self.local)
        both.addWidget(self.remote)
        total.addLayout(both)

        self.show()

    def validate_input(self):
        re_port = QRegExp('\d{1,5}')
        port_validator = QRegExpValidator(re_port, self.portbox)
        self.portbox.setValidator(port_validator)

    def connect(self):
        try:
            host, port = self.hostbox.text(), int(self.portbox.text())
            self.ftp.connect(host, port)
        except Exception as e:
            self.message('Connection refused by server: {}'.format(e), 'red')
            return
        self.message('Connected to server {}:{}'.format(host, port))

        try:
            self.login()
            self.message('Login successful')
        except Exception:
            self.message('Login failed', 'red')
            return

        self.connected = 1
        self.get_remote_filelist()

    def login(self):
        user, pasw = self.userbox.text(), self.paswbox.text()
        try:
            self.ftp.login(user, pasw)
        except Exception as err:
            raise err

    def local_change_dir(self, path):
        path = os.path.abspath(os.path.join(self.local_path, path))
        if not os.path.exists(path) and not os.path.isdir(path):
            return

        self.local_path = path
        self.local.path_edit.setText(path)
        self.get_local_filelist()
        print(self.local_path)

    def local_change_dir_byedit(self):
        self.local_change_dir(self.local.path_edit.text())

    def local_change_dir_item(self, item, column):
        if item.isdir:
            path = os.path.join(self.local_path, str(item.text(0)))
            self.local_change_dir(path)

    def local_delete_file(self):
        item = self.local.tree.currentItem()

        if not item:
            return
        try:
            name = item.text(0)
            path = os.path.join(self.local_path, name)
            if item.isdir:
                rmtree(path)
            else:
                os.remove(path)
            self.message('Removed file/directory {}'.format(name))
        except Exception as e:
            self.message('Removing file/directory {} failed: {}'.format(name, e), 'red')

        self.get_local_filelist()

    def local_rename_file(self):
        item = self.local.tree.currentItem()

        if not item:
            return
        try:
            name = item.text(0)
            path = os.path.join(self.local_path, name)
            toname, pressed = QInputDialog.getText(self, 'Rename To', 'Filename:', QLineEdit.Normal, name)
            if not pressed or not toname:
                return

            os.rename(path, os.path.join(self.local_path, toname))
        except Exception as e:
            self.message('Rename file {} failed: {}'.format(name, e), 'red')

        self.get_local_filelist()

    def upload_file(self, fp, filename, size):
        try:
            self.ftp.store('STOR ' + filename, fp)
            fp.close()
        except Exception as e:
            self.message('Uploading failed: {}'.format(e), 'red')

    def local_upload_selected_item(self):
        item = self.local.tree.currentItem()

        if not item or item.isdir:
            return
        try:
            filename = item.text(0)
            path = os.path.join(self.local_path, filename)
            size = os.path.getsize(path)
            fp = open(path, 'rb')

            def upload_file():
                self.upload_file(fp, filename, size)

            def message():
                self.message('Uploading file {} completed: total {} bytes'.format(filename, size))
                self.get_remote_filelist()

            self.thread = MyThread(upload_file, message)
            self.thread.start()
        except Exception as e:
            self.message('Upload failed: {}'.format(e), 'red')

    def local_mkdir(self):
        try:
            dirname, pressed = QInputDialog.getText(self, 'Create Directory', 'Directory Name:', QLineEdit.Normal)
            if not pressed or not dirname:
                return
            path = os.path.join(self.local_path, dirname)
            os.makedirs(path, exist_ok=True)
        except Exception as e:
            self.message('Creating Directory {} failed: {}'.format(dirname, e), 'red')

        self.get_local_filelist()

    def remote_change_dir(self, path):
        if not self.connected:
            self.connect()
            if not self.connected:
                return

        try:
            self.ftp.cwd(path)
        except Exception as e:
            self.message('Failed to change directory: {}'.format(e))
            return

        self.remote_path = os.path.abspath(path)
        self.remote.path_edit.setText(path)
        self.message('Changed to remote directory: {}'.format(self.remote_path))
        self.get_remote_filelist()
        print(self.remote_path)

    def remote_change_dir_byedit(self):
        self.remote_change_dir(self.remote.path_edit.text())

    def remote_change_dir_item(self, item, column):
        if item.isdir:
            path = os.path.join(self.remote_path, str(item.text(0)))
            self.remote_change_dir(path)

    def remote_delete_file(self):
        item = self.remote.tree.currentItem()

        if not item:
            return
        try:
            name = item.text(0)
            dirpath = os.path.join(self.remote_path, item.text(0))
            if item.isdir:
                self.ftp.rmd(name)
            else:
                self.ftp.dele(name)
            self.message('Removed remote file/directory {}'.format(name))
        except Exception as e:
            self.message('Removing remote file/directory {} failed: {}'.format(name, e), 'red')

        self.get_remote_filelist()

    def remote_rename_file(self):
        item = self.remote.tree.currentItem()

        if not item:
            return
        try:
            name = item.text(0)
            dirpath = os.path.join(self.remote_path, item.text(0))
            toname, pressed = QInputDialog.getText(self, 'Rename To', 'Filename:', QLineEdit.Normal, name)
            if not pressed or not toname:
                return

            self.ftp.rename(name, toname)
            self.message('Renamed file {} to {}'.format(name, toname))
        except Exception as e:
            self.message('Renaming file {} failed: {}'.format(name, e), 'red')

        self.get_remote_filelist()

    def download_file(self, fp, filename, rest=None):
        try:
            self.ftp.retrieve('RETR ' + filename, callback=fp.write, rest=rest)
            fp.close()
        except Exception as e:
            self.message('Uploading failed: {}'.format(e), 'red')

    def remote_download_selected_item(self):
        item = self.remote.tree.currentItem()

        if not item or item.isdir:
            return
        try:
            # local file
            filename = item.text(0)
            size = item.text(1)
            path = os.path.join(self.local_path, filename)

            if os.path.isfile(path):
                # file exist, ask for which action to take
                mode, ok = RadioDialog.getOption(self)
            else:
                mode, ok = 'overwrite', True

            if ok:
                if mode == 'overwrite':
                 open_mode = 'wb'
                else:
                    open_mode = 'ab'
               
                fp = open(path, open_mode)
                rest = None
                if mode == 'rest':
                    # rest should be local file size
                    rest = str(os.path.getsize(path))

                def download_file():
                    self.download_file(fp, filename, rest)

                def message():
                    self.message('Downloading {} complete: total {} bytes'.format(filename, size))
                    self.get_local_filelist()

                self.thread = MyThread(f=download_file, finish=message)
                self.thread.start()
        except Exception as e:
            self.message('Download failed: {}'.format(e), 'red')

    def remote_mkdir(self):
        try:
            dirname, pressed = QInputDialog.getText(self, 'Create Directory', 'Directory Name:', QLineEdit.Normal)
            if not pressed or not dirname:
                return
            self.ftp.mkd(dirname)
        except Exception as e:
            self.message('Creating Remote Directory {} failed: {}'.format(dirname, e), 'red')

        self.get_remote_filelist()

    def get_local_filelist(self):
        self.local.path_edit.setText(self.local_path)
        self.local_complete_list.clear()
        self.local.tree.clear()

        for filename in os.listdir(self.local_path):
            pathname = os.path.join(self.local_path, filename)
            fileinfo = QFileInfo(pathname)
            item = QTreeWidgetItem()

            if fileinfo.isDir():
                icon = QIcon(self.foldericon_path)
                item.isdir = True
            else:
                icon = QIcon(self.fileicon_path)
                item.isdir = False

            size = '' if item.isdir else fileinfo.size()
            time = fileinfo.lastModified().toString()
            owner = fileinfo.owner()
            group = fileinfo.group()

            self.local_complete_list.append(pathname)

            item.setIcon(0, icon)
            for n, i in enumerate((filename, size, time, owner, group)):
                item.setText(n, str(i))

            self.local.tree.addTopLevelItem(item)
            if not self.local.tree.currentItem():
                self.local.tree.setCurrentItem(self.local.tree.topLevelItem(0))
                self.local.tree.setEnabled(True)

        self.local.completer_model.setStringList(self.local_complete_list)

    def parse_fileinfo(self, rawinfo):
        """
        drwxr-xr-x 10 nasen nasen 4096 Oct 14 23:16 Desktop
        """
        items = list(filter(None, rawinfo.split(' ')))
        mode, num, owner, group, size, date, filename = (
            items[0], items[1], items[2], items[3], items[4], ' '.join(items[5:8]), ' '.join(items[8:])
        )
        return mode, num, owner, group, size, date, filename

    def get_remote_filelist(self):
        try:
            remote_path = self.ftp.pwd()
            if not remote_path:
                raise Exception('server does not return 257')
            self.remote_path = remote_path
            self.remote.path_edit.setText(self.remote_path)
            self.remote_complete_list.clear()
            self.remote.tree.clear()

            res = self.ftp.retrieve_result('LIST')
            res = res.splitlines()
            # first line is 'total xxx', last line is empty
            for info in res:
                if not info or info.startswith('total') or info.isspace():
                    continue

                mode, num, owner, group, size, time, filename = self.parse_fileinfo(info)

                item = QTreeWidgetItem()
                if mode[0] == 'd':
                    icon = QIcon(self.foldericon_path)
                    item.isdir = True
                    size = ''
                else:
                    icon = QIcon(self.fileicon_path)
                    item.isdir = False

                item.setIcon(0, icon)
                self.remote_complete_list.append(os.path.join(self.remote_path, filename))

                for n, i in enumerate((filename, size, time, owner, group)):
                    item.setText(n, str(i))

                self.remote.tree.addTopLevelItem(item)
                if not self.remote.tree.currentItem():
                    self.remote.tree.setCurrentItem(self.remote.tree.topLevelItem(0))
                    self.remote.tree.setEnabled(True)

            self.remote.completer_model.setStringList(self.remote_complete_list)
        except Exception as e:
            self.message('Failed to get remote file list: {}'.format(e), 'red')

    def set_mode(self):
        if self.mode_select.currentText() == 'PASV':
            self.ftp.set_pasv(True)
        else:
            self.ftp.set_pasv(False)

    def message(self, text, color='green'):
        if color == 'default':
            self.messagebox.setTextColor(QColor())
            self.messagebox.append(text + '\n')
        else:
            self.messagebox.setTextColor(QColor(color))
            self.messagebox.append(text + '\n')


if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = App()
    sys.exit(app.exec())
