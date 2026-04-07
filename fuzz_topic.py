import time

from fuzz_common import CRASH, EXCEPTION, FAIL, IRCFuzzHarness, PASS


def _nick(prefix="t"):
    return f"{prefix[:4]}{int(time.time() * 1000) % 100000:05d}"


def test_topic_view_and_set(h):
    h.start_server("TOPIC view and set")
    op = None
    result = None
    try:
        op = h.new_client()
        h.auth(op, nick=_nick("top"), user="op")
        _ = h.drain(op)
        h.send_line(op, "JOIN #topic")
        _ = h.drain(op)

        h.send_line(op, "TOPIC #topic")
        out1 = h.drain(op)

        h.send_line(op, "TOPIC #topic :new-topic")
        _ = h.drain(op)
        h.send_line(op, "TOPIC #topic")
        out2 = h.drain(op)

        ok = (h.has_numeric(out1, 331) or h.has_numeric(out1, 332)) and h.has_numeric(
            out2, 332
        )
        result = PASS if ok else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if op:
            op.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_topic_t_mode_non_op_denied(h):
    h.start_server("TOPIC +t non-op denied")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        h.auth(op, nick=_nick("op"), user="op")
        h.auth(user, nick=_nick("u"), user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #topic2")
        h.send_line(user, "JOIN #topic2")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "MODE #topic2 +t")
        _ = h.drain(op)

        h.send_line(user, "TOPIC #topic2 :denied")
        out = h.drain(user)
        result = PASS if h.has_numeric(out, 482) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if op:
            op.close()
        if user:
            user.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_topic_missing_param(h):
    h.start_server("TOPIC missing parameter")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("tm"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "TOPIC")
        out = h.drain(sock)
        result = PASS if h.has_numeric(out, 461) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_topic_not_on_channel(h):
    h.start_server("TOPIC not on channel")
    owner = outsider = None
    result = None
    try:
        owner = h.new_client()
        outsider = h.new_client()
        h.auth(owner, nick=_nick("to"), user="o")
        h.auth(outsider, nick=_nick("tu"), user="u")
        _ = h.drain(owner)
        _ = h.drain(outsider)

        h.send_line(owner, "JOIN #topic3")
        _ = h.drain(owner)

        h.send_line(outsider, "TOPIC #topic3 :change")
        out = h.drain(outsider)
        result = PASS if h.has_numeric(out, 442) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if owner:
            owner.close()
        if outsider:
            outsider.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def run(host="127.0.0.1", port=6667, password="password"):
    h = IRCFuzzHarness(host=host, port=port, password=password)
    return h.apply_leak_overrides(
        {
            "TOPIC view and set": test_topic_view_and_set(h),
            "TOPIC +t non-op denied": test_topic_t_mode_non_op_denied(h),
            "TOPIC missing parameter": test_topic_missing_param(h),
            "TOPIC not on channel": test_topic_not_on_channel(h),
        }
    )


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
