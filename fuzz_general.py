from fuzz_common import CRASH, EXCEPTION, FAIL, IRCFuzzHarness, PASS


def test_unknown_command(h):
    h.start_server("GENERAL unknown command")
    sock = None
    try:
        sock = h.new_client()

        # 先に登録する
        h.send_line(sock, "PASS password")
        h.send_line(sock, "NICK fuzz421")
        h.send_line(sock, "USER fuzz 0 * :fuzz user")
        h.drain(sock)

        # 登録後に未知コマンド
        h.send_line(sock, "THISCOMMANDDOESNOTEXIST")
        out = h.drain(sock)
        return PASS if h.has_numeric(out, 421) else FAIL
    except Exception:
        return EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            return CRASH
        h.stop_server()


def test_not_registered(h):
    h.start_server("GENERAL not registered")
    sock = None
    try:
        sock = h.new_client()
        h.send_line(sock, "JOIN #needreg")
        out = h.drain(sock)
        return PASS if h.has_numeric(out, 451) else FAIL
    except Exception:
        return EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            return CRASH
        h.stop_server()


def run(host="127.0.0.1", port=6667, password="password"):
    h = IRCFuzzHarness(host=host, port=port, password=password)
    return {
        "GENERAL unknown command": test_unknown_command(h),
        "GENERAL not registered": test_not_registered(h),
    }


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
