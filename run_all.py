from fuzz_general import run as run_general
from fuzz_invite import run as run_invite
from fuzz_join import run as run_join
from fuzz_kick import run as run_kick
from fuzz_mode import run as run_mode
from fuzz_nick import run as run_nick
from fuzz_pass import run as run_pass
from fuzz_privmsg import run as run_privmsg
from fuzz_topic import run as run_topic
from fuzz_user import run as run_user


def _print_suite(title, results):
    print(f"\n=== {title} ===")
    for name, status in results.items():
        mark = "✓" if status == "PASS" else "✗"
        print(f"{mark} {name}: {status}")


def main(host="127.0.0.1", port=6667, password="password"):
    suites = [
        ("GENERAL", run_general),
        ("PASS", run_pass),
        ("NICK", run_nick),
        ("USER", run_user),
        ("JOIN", run_join),
        ("PRIVMSG", run_privmsg),
        ("KICK", run_kick),
        ("INVITE", run_invite),
        ("TOPIC", run_topic),
        ("MODE", run_mode),
    ]

    all_results = {}
    for suite_name, runner in suites:
        results = runner(host=host, port=port, password=password)
        _print_suite(suite_name, results)
        all_results[suite_name] = results

    total = 0
    failed = 0
    for suite in all_results.values():
        for status in suite.values():
            total += 1
            if status != "PASS":
                failed += 1

    print("\n=== SUMMARY ===")
    print(f"Total: {total}")
    print(f"Passed: {total - failed}")
    print(f"Failed: {failed}")

    return all_results


if __name__ == "__main__":
    main()
