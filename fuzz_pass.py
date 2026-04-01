import time

from fuzz_common import CRASH, EXCEPTION, FAIL, IRCFuzzHarness, PASS


def _nick(prefix="u"):
    return f"{prefix[:4]}{int(time.time() * 1000) % 100000:05d}"


def test_pass_missing_param(h):
    name = "PASS missing parameter"
    h.start_server(name)
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.send_line(sock, "PASS")
        out = h.drain(sock)
        if h.has_numeric(out, 461):
            result = PASS
        else:
            result = FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_pass_wrong_password(h):
    name = "PASS wrong password"
    h.start_server(name)
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.send_line(sock, "PASS wrong")
        out = h.drain(sock)
        if h.has_numeric(out, 464):
            result = PASS
        else:
            result = FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_pass_then_register(h):
    name = "PASS normal registration"
    h.start_server(name)
    sock = None
    result = None
    try:
        sock = h.new_client()
        nick = _nick("pass")
        h.send_line(sock, f"PASS {h.password}")
        h.send_line(sock, f"NICK {nick}")
        h.send_line(sock, "USER test 0 * :Pass Test")
        out = h.drain(sock)
        if h.has_numeric(out, 1):
            result = PASS
        else:
            result = FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_pass_after_registered(h):
    name = "PASS already registered"
    h.start_server(name)
    sock = None
    result = None
    try:
        sock = h.new_client()
        nick = _nick("passr")
        h.auth(sock, nick=nick, user="u")
        _ = h.drain(sock)
        h.send_line(sock, f"PASS {h.password}")
        out = h.drain(sock)
        if h.has_numeric(out, 462):
            result = PASS
        else:
            result = FAIL
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
        "PASS missing parameter": test_pass_missing_param(h),
        "PASS wrong password": test_pass_wrong_password(h),
        "PASS normal registration": test_pass_then_register(h),
        "PASS already registered": test_pass_after_registered(h),
    }


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
