import os
import re
import socket
import subprocess
import time
from datetime import datetime
from typing import Dict

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
        self.current_test_name = None
        self.current_valgrind_log_path = None
        self.last_valgrind_report = None
        self.leak_by_test = {}

        # FUZZ_VALGRIND=1 で有効化
        self.use_valgrind = os.getenv("FUZZ_VALGRIND", "0") == "1"
        self.valgrind_bin = os.getenv("FUZZ_VALGRIND_BIN", "valgrind")
        # ここでは "still reachable" は失敗扱いにしない
        self.treat_still_reachable_as_leak = (
            os.getenv("FUZZ_VALGRIND_REACHABLE", "0") == "1"
        )
        self.startup_timeout = float(
            os.getenv(
                "FUZZ_SERVER_STARTUP_TIMEOUT", "10.0" if self.use_valgrind else "2.0"
            )
        )

    def _valgrind_command(self, valgrind_log_path):
        return [
            self.valgrind_bin,
            "--leak-check=full",
            "--show-leak-kinds=all",
            "--track-origins=yes",
            "--error-exitcode=99",
            f"--log-file={valgrind_log_path}",
            self.server_path,
            str(self.port),
            self.password,
        ]

    @staticmethod
    def _parse_valgrind_bytes(line, key):
        m = re.search(rf"{re.escape(key)}:\\s*([0-9,]+)\\s+bytes", line)
        if not m:
            return 0
        return int(m.group(1).replace(",", ""))

    def _parse_valgrind_report(self, text):
        definitely = self._parse_valgrind_bytes(text, "definitely lost")
        indirectly = self._parse_valgrind_bytes(text, "indirectly lost")
        possibly = self._parse_valgrind_bytes(text, "possibly lost")
        reachable = self._parse_valgrind_bytes(text, "still reachable")

        leak = (definitely + indirectly + possibly) > 0
        if self.treat_still_reachable_as_leak:
            leak = leak or reachable > 0

        return {
            "definitely_lost": definitely,
            "indirectly_lost": indirectly,
            "possibly_lost": possibly,
            "still_reachable": reachable,
            "has_leak": leak,
        }

    def _collect_valgrind_report(self):
        if not self.current_valgrind_log_path:
            self.last_valgrind_report = None
            return None

        try:
            with open(
                self.current_valgrind_log_path, "r", encoding="utf-8", errors="replace"
            ) as fp:
                content = fp.read()
        except Exception:
            self.last_valgrind_report = None
            return None

        report = self._parse_valgrind_report(content)
        self.last_valgrind_report = report

        if self.current_test_name:
            self.leak_by_test[self.current_test_name] = bool(report.get("has_leak"))

        return report

    def _wait_server_ready(self):
        deadline = time.time() + self.startup_timeout
        last_err = None

        while time.time() < deadline:
            if self.server_proc and self.server_proc.poll() is not None:
                raise RuntimeError(
                    f"Server exited during startup (exit={self.server_proc.returncode})"
                )

            probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            probe.settimeout(0.2)
            try:
                probe.connect((self.host, self.port))
                probe.close()
                return
            except Exception as e:
                last_err = e
                time.sleep(0.05)
            finally:
                try:
                    probe.close()
                except Exception:
                    pass

        raise TimeoutError(
            f"Server did not become ready within {self.startup_timeout:.1f}s: {last_err}"
        )

    def _slugify(self, text):
        return "".join(c.lower() if c.isalnum() else "_" for c in text).strip("_")

    def _ensure_server_binary(self):
        if os.path.isfile(self.server_path) and os.access(self.server_path, os.X_OK):
            return

        subprocess.check_call(["make"], cwd=self.server_cwd)

        if not (
            os.path.isfile(self.server_path) and os.access(self.server_path, os.X_OK)
        ):
            raise FileNotFoundError(
                f"Server binary was not created at {self.server_path} after running make"
            )

    def start_server(self, test_name):
        self.current_test_name = test_name
        self.current_valgrind_log_path = None
        self.last_valgrind_report = None

        os.makedirs(self.log_dir, exist_ok=True)
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        log_path = os.path.join(self.log_dir, f"{ts}_{self._slugify(test_name)}.log")
        self.server_log_fp = open(log_path, "w", encoding="utf-8")
        self.server_log_fp.write(f"[TEST] {test_name}\n")
        self.server_log_fp.write(f"[START] {datetime.now().isoformat()}\n")
        self.server_log_fp.flush()

        self._ensure_server_binary()

        cmd = [self.server_path, str(self.port), self.password]
        if self.use_valgrind:
            self.current_valgrind_log_path = os.path.join(
                self.log_dir,
                f"{ts}_{self._slugify(test_name)}.valgrind.log",
            )
            cmd = self._valgrind_command(self.current_valgrind_log_path)

        self.server_log_fp.write(f"[CMD] {' '.join(cmd)}\n")
        self.server_log_fp.flush()

        self.server_proc = subprocess.Popen(
            cmd,
            cwd=self.server_cwd,
            stdout=self.server_log_fp,
            stderr=subprocess.STDOUT,
        )
        self._wait_server_ready()
        return log_path

    def stop_server(self):
        if self.server_proc:
            self.server_proc.terminate()
            try:
                self.server_proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.server_proc.kill()
            self.server_proc = None

        vg = self._collect_valgrind_report()

        if self.server_log_fp:
            if vg is not None:
                self.server_log_fp.write(
                    "[VALGRIND] definitely_lost={} indirectly_lost={} possibly_lost={} still_reachable={} has_leak={}\n".format(
                        vg["definitely_lost"],
                        vg["indirectly_lost"],
                        vg["possibly_lost"],
                        vg["still_reachable"],
                        vg["has_leak"],
                    )
                )
            self.server_log_fp.write(f"[END] {datetime.now().isoformat()}\n")
            self.server_log_fp.flush()
            self.server_log_fp.close()
            self.server_log_fp = None

        return vg

    def apply_leak_overrides(self, results: Dict[str, str]) -> Dict[str, str]:
        if not self.use_valgrind:
            return results

        adjusted = {}
        for name, status in results.items():
            if status == "PASS" and self.leak_by_test.get(name, False):
                adjusted[name] = "LEAK"
            else:
                adjusted[name] = status
        return adjusted

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
