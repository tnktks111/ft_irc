import time

from fuzz_common import CRASH, EXCEPTION, FAIL, IRCFuzzHarness, PASS


def _nick(prefix="n"):
    return f"{prefix[:4]}{int(time.time() * 1000) % 100000:05d}"


def test_nick_missing_param(h):
    h.start_server("NICK missing parameter")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.send_line(sock, f"PASS {h.password}")
        h.send_line(sock, "NICK")
        out = h.drain(sock)
        result = PASS if h.has_numeric(out, 431) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_nick_duplicate(h):
    h.start_server("NICK duplicate")
    a = b = None
    result = None
    try:
        nick = _nick("dup")
        a = h.new_client()
        h.auth(a, nick=nick, user="a")
        _ = h.drain(a)

        b = h.new_client()
        h.send_line(b, f"PASS {h.password}")
        h.send_line(b, f"NICK {nick}")
        out = h.drain(b)
        result = PASS if h.has_numeric(out, 433) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if a:
            a.close()
        if b:
            b.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_nick_erroneous(h):
    h.start_server("NICK erroneous nickname (RFC2812)")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.send_line(sock, f"PASS {h.password}")

        # RFC2812 2.3.1: nickname = ( letter / special ) *8( letter / digit / special / "-" )
        # 1) starts with digit -> invalid
        h.send_line(sock, "NICK 1badnick")
        out1 = h.drain(sock)

        # 2) longer than 9 chars -> invalid
        h.send_line(sock, "NICK abcdefghij")
        out2 = h.drain(sock)

        ok = h.has_numeric(out1, 432) and h.has_numeric(out2, 432)
        result = PASS if ok else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def run(host="127.0.0.1", port=6667, password="password"):
    h = IRCFuzzHarness(host=host, port=port, password=password)
    return {
        "NICK missing parameter": test_nick_missing_param(h),
        "NICK duplicate": test_nick_duplicate(h),
        "NICK erroneous nickname (RFC2812)": test_nick_erroneous(h),
    }


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
