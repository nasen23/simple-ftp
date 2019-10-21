import re
import sys
import socket
from socket import _GLOBAL_DEFAULT_TIMEOUT


# constants #
MAX_LINE = 8192

FTP_PORT = 21

CRLF = '\r\n'
CRLF_B = b'\r\n'


# response parsing functions #
_re_addr = None

def parse_addr(res):
    """
    Parse the response from 'PASV'.
    The response is like (h1,h2,h3,h4,p1,p2).
    Raise an error_proto if response doesn't contain the excepted addr.
    """
    global _re_addr
    if not _re_addr:
        _re_addr = re.compile(r'(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)', re.ASCII)
    match = _re_addr.search(res)
    if not match:
        raise error_proto(res)
    nums = match.groups()
    host = '.'.join(nums[:4])
    port = (int(nums[4]) << 8) + int(nums[5])
    return host, port

def parse_dirname(res):
    """
    Parse the response from 'MKD' or 'PWD'.
    """
    if res[:3] != '257':
        raise error_perm(res)
    if res[3:5] != ' "':
        return ''

    # TODO: parse with trailing double quotes
    dirname = ''
    cur = 5
    res_len = len(res)
    while cur < res_len:
        if res[cur] == '"':
            break
        dirname += res[cur]
        cur += 1

    return dirname


class ftp_error(Exception):
    pass

# errors #
class error_reply(ftp_error):
    """
    unexpected [123]xx reply
    """
    pass

class error_temp(ftp_error):
    """
    4xx error
    """
    pass

class error_perm(ftp_error):
    """
    5xx error
    """
    pass

class error_proto(ftp_error):
    """
    response does not begin with [1-5]
    """
    pass


# a simple ftp client #
class FtpClient(object):
    """
    A ftp client.
    """

    host = ''
    port = FTP_PORT
    maxline = MAX_LINE
    sock = None
    file = None
    welcome = None
    passive = 1
    encoding = 'utf-8'

    def __init__(self, host='', port=0, user='', pasw='', timeout=_GLOBAL_DEFAULT_TIMEOUT):
        self.timeout = timeout
        if host:
            self.connect(host, port)
            if user:
                self.login(user, pasw)

    def connect(self, host='', port=0, timeout=-999):
        if host:
            self.host = host
        if port > 0:
            self.port = port
        if timeout != -999:
            self.timeout = timeout
        self.sock = socket.create_connection((self.host, self.port), self.timeout)
        self.welcome = self.get_res()
        return self.welcome

    def get_line(self):
        """
        Read one-line response from server, return the string with CRLF ripped.
        """
        line = self.sock.recv(self.maxline).decode()
        if len(line) > self.maxline:
            raise Exception('got more than {} bytes in single line'.format(self.maxline))
        if not line:
            raise EOFError
        line = line.rstrip()
        return line

    def get_res(self):
        res = self.get_line()

        self.res = res[:3]
        c = res[:1]
        if c in ('1', '2', '3'):
            return res
        if c == '4':
            raise error_temp(res)
        if c == '5':
            raise error_perm(res)
        raise error_proto(res)

    def get_res_strict(self):
        """
        response must begin with '2'
        """
        res = self.get_line()
        self.res = res[:3]
        if res[:1] != '2':
            raise error_reply(res)
        return res

    def send_line(self, line):
        """
        Send message with CRLF to server.
        """
        line = line.rstrip()
        line += CRLF
        self.sock.sendall(line.encode(self.encoding))

    def send_cmd(self, cmd):
        """Send a command and return its response."""
        self.send_line(cmd)
        return self.get_res()

    def send_cmd_strict(self, cmd):
        """Send a command and expect a response starting with '2' from server."""
        self.send_line(cmd)
        return self.get_res_strict()

    def login(self, user='', pasw=''):
        """Login with user and pasw.
        User is 'anonymous' by default.
        """
        if not user:
            user = 'anonymous'
        if user == 'anonymous' and not pasw:
            pasw += 'anonymous@'
        res = self.send_cmd('USER ' + user)
        if res[0] == '3':
            res = self.send_cmd_strict('PASS ' + pasw)

        return res

    def create_server(self, host, port, family):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.bind((host, port))
        sock.listen(1)

        return sock

    def send_port(self, host, port):
        """Send 'PORT' command with 'host' and 'port'."""
        host = host.split('.')
        port = list(map(str, [port // 256, port % 256]))
        nums = host + port
        cmd = 'PORT {}'.format(','.join(nums))
        return self.send_cmd_strict(cmd)

    def make_port(self):
        """Create data socket and send a port command."""
        sock = self.create_server("", 0, family=socket.AF_INET)
        # get proper port
        port = sock.getsockname()[1]
        # get proper host
        host = self.sock.getsockname()[0]
        res = self.send_port(host, port)
        # if self.timeout is not _GLOBAL_DEFAULT_TIMEOUT:
        #     sock.settimeout(self.timeout)
        return sock

    def make_pasv(self):
        res = self.send_cmd('PASV')
        host, port = parse_addr(res)
        return host, port

    def set_pasv(self, ispasv):
        self.passive = 1 if ispasv else 0

    def init_dataconn(self, cmd, rest=None):
        """
        Initialize data socket and return the connection.
        This could be used for 'LIST', 'RETR', 'STOR'.
        """
        if self.passive:
            host, port = self.make_pasv()
            print(host, port)
            conn = socket.create_connection((host, port), self.timeout)
            try:
                print(rest)
                if rest is not None:
                    self.send_cmd('REST ' + rest)
                res = self.send_cmd(cmd)
                print('res: ' + res)
            except:
                conn.close()
                raise
        else:
            sock = self.make_port()
            if rest is not None:
                self.send_cmd('REST ' + rest)
            res = self.send_cmd(cmd)
            conn, sockaddr = sock.accept()
            # conn.settimeout(self.timeout)
            sock.close()
        return conn

    def retrieve(self, cmd, callback, maxblock=8192, rest=None):
        """
        Retrieve data received in data socket in binary mode.
        Could be used for 'LIST', 'RETR'.
        """
        self.send_cmd_strict('TYPE I')
        conn = self.init_dataconn(cmd, rest)
        while True:
            data = conn.recv(maxblock)
            if not data:
                break
            callback(data)

        conn.close()
        self.get_res_strict()
        return self.res

    def retrieve_result(self, cmd, maxblock=8192, rest=None):
        self.send_cmd_strict('TYPE I')
        conn = self.init_dataconn(cmd, rest)

        fragments = []
        while True:
            data = conn.recv(maxblock)
            if not data:
                break
            fragments.append(data.decode())

        conn.close()
        self.get_res_strict()
        return ''.join(fragments)

    def store(self, cmd, fp, maxblock=8192, rest=None):
        self.send_cmd_strict('TYPE I')
        conn = self.init_dataconn(cmd, rest)
        while True:
            data = fp.read(maxblock)
            if not data:
                break
            conn.sendall(data)

        conn.close()
        self.get_res_strict()
        return self.res

    def rename(self, fromname, toname):
        res = self.send_cmd('RNFR ' + fromname)
        if res[0] != '3':
            raise error_reply(res)

        return self.send_cmd_strict('RNTO ' + toname)

    def dele(self, filename):
        res = self.send_cmd('DELE ' + filename)
        if res[:3] in {'200', '250'}:
            return res
        else:
            raise error_reply(res)

    def cwd(self, dirname):
        if dirname == '..':
            try:
                return self.send_cmd_strict('CDUP')
            except error_perm as msg:
                if msg.args[0][:3] != '500':
                    raise
        elif dirname == '':
            dirname = '.'
        cmd = 'CWD ' + dirname
        return self.send_cmd_strict(cmd)

    def mkd(self, dirname):
        res = self.send_cmd_strict('MKD ' + dirname)
        if not res.startswith('257'):
            return ''

        return parse_dirname(res)

    def rmd(self, dirname):
        return self.send_cmd_strict('RMD ' + dirname)

    def pwd(self):
        res = self.send_cmd_strict('PWD')
        print('res:' + res)
        if not res.startswith('257'):
            return ''
        return parse_dirname(res)

    def quit(self):
        res = self.send_cmd_strict('QUIT')
        # close socket connection
        return res

    def close(self):
        try:
            file = self.file
            self.file = None
            if file is not None:
                file.close()
        finally:
            sock = self.sock
            self.sock = None
            if sock is not None:
                sock.close()


def test():
    client = FtpClient('127.0.0.1', 6789, user='anonymous',
                       pasw='token')
    client.retrieve('LIST', callback=print)

if __name__ == '__main__':
    test()
