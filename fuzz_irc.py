#!/usr/bin/env python3
import argparse
import random
import socket
import string
import subprocess
import time
from datetime import datetime
from pathlib import Path

import psutil


def random_ascii(rng: random.Random, min_len: int = 1, max_len: int = 64) -> str:
    length = rng.randint(min_len, max_len)
    alphabet = string.ascii_letters + string.digits + "[]{}^_-|!@#$%&*()~"
    return "".join(rng.choice(alphabet) for _ in range(length))


def build_payload(rng: random.Random) -> bytes:
    mode = rng.choice(
        [
            "validish",
            "oversized",
            "no_crlf",
            "bad_newline",
            "nul_byte",
            "utf8_garbage",
        ]
    )

    if mode == "validish":
        cmd = rng.choice(["JOIN", "PRIVMSG", "MODE", "TOPIC", "KICK", "INVITE"])
        if cmd == "JOIN":
            line = f"JOIN #{random_ascii(rng, 1, 16)}\r\n"
        elif cmd == "PRIVMSG":
            line = f"PRIVMSG #{random_ascii(rng, 1, 12)} :{random_ascii(rng, 1, 128)}\r\n"
        elif cmd == "MODE":
            line = f"MODE #{random_ascii(rng, 1, 12)} +{rng.choice('itkolz')} {random_ascii(rng, 0, 16)}\r\n"
        elif cmd == "TOPIC":
            line = f"TOPIC #{random_ascii(rng, 1, 12)} :{random_ascii(rng, 0, 128)}\r\n"
        elif cmd == "KICK":
            line = f"KICK #{random_ascii(rng, 1, 12)} {random_ascii(rng, 1, 12)} :{random_ascii(rng, 1, 32)}\r\n"
        else:
            line = f"INVITE {random_ascii(rng, 1, 12)} #{random_ascii(rng, 1, 12)}\r\n"
        return line.encode("utf-8", errors="replace")

    if mode == "oversized":
        huge = random_ascii(rng, 4096, 16384)
        line = f"PRIVMSG #x :{huge}\r\n"
        return line.encode("utf-8", errors="replace")

    if mode == "no_crlf":
        line = f"PRIVMSG #x :{random_ascii(rng, 1024, 8192)}"
        return line.encode("utf-8", errors="replace")

    if mode == "bad_newline":
        line = f"NICK {random_ascii(rng, 1, 32)}\r\r\nUSER x 0 * :y\n\r"
        return line.encode("utf-8", errors="replace")

    if mode == "nul_byte":
        prefix = f"PRIVMSG #x :{random_ascii(rng, 1, 32)}".encode("utf-8", errors="replace")
        return prefix + b"\x00" + random_ascii(rng, 1, 32).encode("utf-8") + b"\r\n"

    # utf8_garbage
    garbage = bytes(rng.getrandbits(8) for _ in range(rng.randint(64, 512)))
    return b"PRIVMSG #x :" + garbage + b"\r\n"


def auth(sock: socket.socket, password: str, nick: str) -> None:
    sock.sendall(f"PASS {password}\r\n".encode())
    sock.sendall(f"NICK {nick}\r\n".encode())
    sock.sendall(f"USER {nick} 0 * :{nick}\r\n".encode())


def start_server(server_dir: Path, port: int, password: str, log_path: Path) -> subprocess.Popen:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    fp = log_path.open("w", encoding="utf-8")
    proc = subprocess.Popen(
        ["./ircserv", str(port), password],
        cwd=str(server_dir),
        stdout=fp,
        stderr=subprocess.STDOUT,
    )
    time.sleep(0.4)
    if proc.poll() is not None:
        raise RuntimeError("ircserv failed to start. Check log file.")
    return proc


def stop_server(proc: subprocess.Popen) -> None:
    if proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        proc.kill()


def main() -> int:
    parser = argparse.ArgumentParser(description="Simple black-box fuzzer for ft_irc")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=6667)
    parser.add_argument("--password", default="password")
    parser.add_argument("--iterations", type=int, default=500)
    parser.add_argument("--seed", type=int, default=None)
    parser.add_argument("--timeout", type=float, default=0.5)
    args = parser.parse_args()

    rng = random.Random(args.seed)
    server_dir = Path(__file__).resolve().parent
    logs_dir = server_dir / "logs"
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    server_log = logs_dir / f"fuzz_server_{ts}.log"
    case_log = logs_dir / f"fuzz_cases_{ts}.bin"

    if not (server_dir / "ircserv").exists():
        print("[!] ./ircserv が見つかりません。先に ft_irc で make してください。")
        return 2

    proc = start_server(server_dir, args.port, args.password, server_log)
    pinfo = psutil.Process(proc.pid)
    print(f"[+] server pid={proc.pid}, seed={args.seed}")

    try:
        for i in range(1, args.iterations + 1):
            payload = build_payload(rng)
            with case_log.open("ab") as fp:
                fp.write(f"\n--- CASE {i} ---\n".encode())
                fp.write(payload)
                fp.write(b"\n")

            try:
                with socket.create_connection((args.host, args.port), timeout=args.timeout) as sock:
                    sock.settimeout(args.timeout)
                    if rng.random() < 0.7:
                        auth(sock, args.password, f"fz{i}")
                    sock.sendall(payload)
                    try:
                        _ = sock.recv(4096)
                    except socket.timeout:
                        pass
            except OSError:
                # 接続失敗もクラッシュ判定前に許容
                pass

            if proc.poll() is not None:
                print(f"[!] CRASH detected at case {i}")
                print(f"[!] server log: {server_log}")
                print(f"[!] payload log: {case_log}")
                return 1

            if i % 50 == 0:
                mem_mb = pinfo.memory_info().rss / 1024 / 1024
                print(f"[*] case={i}, rss={mem_mb:.2f} MB")

        print("[+] fuzzing completed without server crash")
        print(f"[*] server log: {server_log}")
        print(f"[*] payload log: {case_log}")
        return 0
    finally:
        stop_server(proc)


if __name__ == "__main__":
    raise SystemExit(main())
