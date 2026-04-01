import os
import re
import socket
import subprocess
import time
from datetime import datetime

import psutil


class IRCFuzzHarness:
    def __init__(
        self,
        host="127.0.0.1",
        port=6667,
        password="password",
        timeout=2.0,
        server_cwd=None,
        log_dir=None,
    ):
        project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
        self.host = host
        self.port = port
        self.password = password
        self.timeout = timeout
        self.server_cwd = server_cwd or project_root
        self.log_dir = log_dir or os.path.join(project_root, "logs")
        self.server_path = os.path.join(self.server_cwd, "ircserv")
        self.server_proc = None
        self.server_log_fp = None

    def _slugify(self, text):
        return "".join(c.lower() if c.isalnum() else "_" for c in text).strip("_")

    def _ensure_server_binary(self):
        if os.path.isfile(self.server_path) and os.access(self.server_path, os.X_OK):
            return

        subprocess.check_call(["make"], cwd=self.server_cwd)

        if not (os.path.isfile(self.server_path) and os.access(self.server_path, os.X_OK)):
            raise FileNotFoundError(
                f"Server binary was not created at {self.server_path} after running make"
            )

    def start_server(self, test_name):
        os.makedirs(self.log_dir, exist_ok=True)
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        log_path = os.path.join(self.log_dir, f"{ts}_{self._slugify(test_name)}.log")
        self.server_log_fp = open(log_path, "w", encoding="utf-8")
        self.server_log_fp.write(f"[TEST] {test_name}\n")
        self.server_log_fp.write(f"[START] {datetime.now().isoformat()}\n")
        self.server_log_fp.flush()

        self._ensure_server_binary()

        self.server_proc = subprocess.Popen(
            [self.server_path, str(self.port), self.password],
            cwd=self.server_cwd,
            stdout=self.server_log_fp,
            stderr=subprocess.STDOUT,
        )
        time.sleep(0.3)
        return log_path

    def stop_server(self):
        if self.server_proc:
            self.server_proc.terminate()
            try:
                self.server_proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.server_proc.kill()
            self.server_proc = None

        if self.server_log_fp:
            self.server_log_fp.write(f"[END] {datetime.now().isoformat()}\n")
            self.server_log_fp.flush()
            self.server_log_fp.close()
            self.server_log_fp = None

    def is_server_alive(self):
        return bool(self.server_proc and self.server_proc.poll() is None)

    def get_server_memory_mb(self):
        if not self.server_proc:
            return -1.0
        try:
            proc = psutil.Process(self.server_proc.pid)
            return proc.memory_info().rss / 1024 / 1024
        except Exception:
            return -1.0

    def new_client(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(self.timeout)
        sock.connect((self.host, self.port))
        return sock

    def send_line(self, sock, line):
        if not line.endswith("\r\n"):
            line += "\r\n"
        sock.sendall(line.encode("utf-8", errors="replace"))

    def send_raw(self, sock, payload):
        sock.sendall(payload)

    def auth(self, sock, nick="testuser", user="test", realname="Test User"):
        self.send_line(sock, f"PASS {self.password}")
        self.send_line(sock, f"NICK {nick}")
        self.send_line(sock, f"USER {user} 0 * :{realname}")
        time.sleep(0.2)

    def drain(self, sock, duration=0.5):
        end = time.time() + duration
        chunks = []
        while time.time() < end:
            try:
                data = sock.recv(4096)
                if not data:
                    break
                chunks.append(data)
            except socket.timeout:
                break
            except BlockingIOError:
                break
        return b"".join(chunks).decode("utf-8", errors="replace")

    @staticmethod
    def has_numeric(payload, code):
        return (
            re.search(rf"(^|\s){int(code):03d}(\s|$)", payload, re.MULTILINE)
            is not None
        )
