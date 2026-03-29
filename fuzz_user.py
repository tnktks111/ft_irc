import time

from fuzz_common import CRASH, EXCEPTION, FAIL, IRCFuzzHarness, PASS


def _nick(prefix="u"):
    return f"{prefix}{int(time.time() * 1000) % 100000}"


def test_user_missing_param(h):
    h.start_server("USER missing parameter")
    sock = None
    try:
        sock = h.new_client()
        h.send_line(sock, f"PASS {h.password}")
        h.send_line(sock, f"NICK {_nick('usr')}")
        h.send_line(sock, "USER short 0 *")
        out = h.drain(sock)
        return PASS if h.has_numeric(out, 461) else FAIL
    except Exception:
        return EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            return CRASH
        h.stop_server()


def test_user_already_registered(h):
    h.start_server("USER already registered")
    sock = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("reg"), user="u1")
        _ = h.drain(sock)
        h.send_line(sock, "USER again 0 * :Again")
        out = h.drain(sock)
        return PASS if h.has_numeric(out, 462) else FAIL
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
        "USER missing parameter": test_user_missing_param(h),
        "USER already registered": test_user_already_registered(h),
    }


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
