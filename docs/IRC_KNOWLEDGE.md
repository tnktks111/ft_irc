# ft_irc — 必要知識まとめ

subject (`en.subject.pdf`) と以下 3 つの RFC を根拠にまとめた実装ガイド。

- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459) : 原典
- [RFC 2811 — Internet Relay Chat: Channel Management](https://datatracker.ietf.org/doc/rfc2811/) : チャネル管理仕様
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812) : クライアントプロトコル(1459 の更新版)

**注意:** 現代の IRC クライアントは 2812/2811 を基準にしていることが多いので、実装は基本的に **2812/2811 に合わせる** のが安全。1459 は原典として参考にする。

---

## 0. 課題の要件(subject要約)

| 項目 | 要件 |
|---|---|
| プログラム名 | `ircserv` |
| 起動方法 | `./ircserv <port> <password>` |
| 言語 | C++98(`-Wall -Wextra -Werror -std=c++98`) |
| 禁止 | 外部ライブラリ / Boost / fork / server-to-server 通信 / クライアント実装 |
| 必須 | ノンブロッキング I/O / `poll()` は **1 回だけ**(全 fd の read/write/listen を一元管理) |
| MacOS のみ | `fcntl(fd, F_SETFL, O_NONBLOCK)` **だけ** 例外的に許可 |
| 依拠クライアント | 1 つの reference client を選び、それに合わせて動作させる(評価時に使う) |
| プロトコル | TCP/IP(v4 or v6) |
| クラッシュ禁止 | どんな状況でも(OOM 含む)クラッシュ・予期しない終了は 0 点 |
| errno 参照禁止 | `read/recv/write/send` 後に `errno`(EAGAIN 等)を見て判断すると 0 点 |
| 実装必須機能 | 認証 / NICK / USER / JOIN / PRIVMSG / operator 概念 |
| 必須オペレータ機能 | KICK, INVITE, TOPIC, MODE(**i, t, k, l, o** の 5 つ) |
| bonus | ファイル転送(DCC) / bot 。mandatory が **PERFECT** でないと採点されない |

**重要な運用上の落とし穴:**
- `nc -C 127.0.0.1 6667` で `com`^D`man`^D`d\n` のように分割送信された場合でも 1 つのコマンドとして処理できないと **0 点**。→ 受信データは `\r\n` が現れるまでバッファに蓄積し、行が完成してから初めてパースする。
- ソケットは常に non-blocking。読み書きの可否は `poll()` の `POLLIN`/`POLLOUT` だけで判断する。`EAGAIN` を見てリトライしない。

---

## 1. 全体アーキテクチャ

シングルスレッド + シングル `poll()` のイベントループが基本。

```
              ┌─────────┐   accept()      ┌────────────┐
listen fd ──▶ │  poll() │ ───────────────▶│ new Client │
              │  ループ  │◀─── POLLIN ─────│  (fd 追加) │
              └────┬────┘   POLLOUT       └────────────┘
                   │
       ┌───────────┼──────────────┐
       ▼           ▼              ▼
   parse msg   dispatch      write sendBuf
   (\r\n split) command→handler   (部分送信対応)
```

### 主要データ構造(このリポジトリの構成)

| クラス | 役割 |
|---|---|
| `Server` | listen socket 管理 + `poll()` ループ + fd→Client の対応表 |
| `ServerContext` | サーバー起動時の設定(port, password, hostname 等)を持つ |
| `Client` | 1 接続の状態(fd, nick, user, real, host, 受信/送信バッファ, 登録状態) |
| `Channel` | チャネル状態(name, topic, members, operators, invite list, modes) |
| `Message` | パース済みメッセージ(prefix, command, params) |
| `CommandDispatcher` | command 名 → handler へディスパッチ |
| `ACommand` + 各 `*Command` | コマンド 1 つずつのハンドラ(Command パターン) |
| `ReplyBuilder` | 数字応答(001, 332, 353, 401 …)の文字列生成 |
| `ResponseSink` | 送信バッファへの書き込み一元化 |
| `MsgTargetResolver` | PRIVMSG などの `<msgtarget>` を User/Channel に解決 |
| `IrcCaseMapping` / `HostCaseMapping` | Nick/Channel の case-insensitive 比較(RFC 1459 の scandinavian 規則) |

---

## 2. メッセージフォーマット(RFC 1459 §2.3 / 2812 §2.3)

```
message    = [ ':' prefix SPACE ] command [ params ] CR LF
prefix     = servername / ( nickname [ '!' user ] [ '@' host ] )
command    = 1*letter / 3digit
params     = *14( SPACE middle ) [ SPACE ':' trailing ]
           / 14( SPACE middle ) [ SPACE [':'] trailing ]
middle     = nospcrlfcl *( ':' / nospcrlfcl )   ; ':' で始まらない
trailing   = *( ':' / ' ' / nospcrlfcl )        ; ':' や ' ' を含める
```

### 絶対に押さえる 5 点

1. **1 メッセージ最大 512 byte(CR-LF 含む)**。つまり本文は 510 byte まで。
2. **必ず `\r\n` で終端**。ただし現代クライアント/nc は `\n` だけで送ってくることがあるので、**受信側は `\r\n` と `\n` の両方を許容する** のが実践的。送信は必ず `\r\n`。
3. **params は最大 15 個**(middle × 最大 14 + trailing 1)。
4. `:` で始まる param は **trailing** で、以降は空白を含めて 1 つの param として扱う(必ず最後の param)。
5. サーバー発 message には prefix `:server.name` を付ける。ユーザー由来の中継(JOIN 通知など)は `:nick!user@host` を付ける。

### 具体例

| 受信 | パース結果 |
|---|---|
| `NICK alice\r\n` | cmd=`NICK`, params=[`alice`] |
| `USER u 0 * :Real Name\r\n` | cmd=`USER`, params=[`u`,`0`,`*`,`Real Name`] |
| `PRIVMSG #ch :hello, world\r\n` | cmd=`PRIVMSG`, params=[`#ch`,`hello, world`] |
| `JOIN #a,#b key1,key2\r\n` | cmd=`JOIN`, params=[`#a,#b`,`key1,key2`] |

| サーバー→クライアント送信例 | 説明 |
|---|---|
| `:irc.myserv 001 alice :Welcome to ...\r\n` | 数字応答 001 |
| `:alice!u@h JOIN :#foo\r\n` | alice が #foo に参加した通知 |
| `:server PING :token\r\n` | PING |

---

## 3. Nickname / Channel / Username の文法

### Nickname (RFC 2812 §2.3.1)
```
nickname = ( letter / special ) *8( letter / digit / special / '-' )
letter   = A..Z / a..z
digit    = 0..9
special  = '[' / ']' / '\' / '`' / '_' / '^' / '{' / '|' / '}'
```
- **最大 9 文字**(RFC 2812。ただし現代の実装は 15〜30 文字許容が多い。課題では 9 で揃えるのが無難)
- 先頭は letter または special。ハイフンは 2 文字目以降のみ。
- 大文字小文字の等価性(scandinavian rule):
  `{}|` は `[]\` の小文字扱い。加えて `^` は `~` の小文字扱い(2812)。
  → `NICK Alice` と `NICK alice` は同一。比較は必ずこの規則で行う。

### Channel name (RFC 2812 §2.3.1 / RFC 2811 §2)
```
channel = ( '#' / '+' / '!' channelid / '&' ) chanstring [ ':' chanstring ]
```
- **先頭文字**: `#` (network-wide), `&` (server-local), `+` (modeless), `!` (safe)
  - 課題では **`#` と `&` だけ受ければ十分**(2811 の `+`/`!` は評価対象外のことが多い)
- **最大 50 文字**
- **禁止文字**: 空白, `\a`(BEL, 0x07), カンマ `,`(target 区切り), コロン `:`(mask 区切り), CR, LF
- 大文字小文字を **区別しない**(nick と同じ規則)

### Username (RFC 2812 §2.3.1)
```
user = 1*( %x01-09 / %x0B-0C / %x0E-1F / %x21-3F / %x41-FF )
```
- NUL, CR, LF, SPACE, `@` を除く任意の 8bit 文字
- 実装上は簡単のため letter/digit/一部 special に絞ってよい

---

## 4. 接続と登録シーケンス

クライアントは TCP 接続直後に以下を **この順序で** 送ってくる。

```
1. PASS <password>           (サーバー要求時のみ。任意)
2. NICK <nickname>
3. USER <user> <mode> <unused> :<realname>
```

- `PASS` は **NICK/USER より前**。順序違反は ERR_ALREADYREGISTRED(462)。
- 全て揃うまで `_isRegistered = false`。揃った瞬間に:
  - `001 RPL_WELCOME`
  - `002 RPL_YOURHOST`
  - `003 RPL_CREATED`
  - `004 RPL_MYINFO`
  - (任意) MOTD 375/372/376 or ERR_NOMOTD 422
  を順番に送信する。
- パスワード違反は `464 ERR_PASSWDMISMATCH` を返して切断。

**登録前に受け付けてよいコマンド:** `PASS`, `NICK`, `USER`, `QUIT`, `CAP`(無視可)。それ以外は `451 ERR_NOTREGISTERED`。

**登録後の `PASS`/`USER` 再送:** `462 ERR_ALREADYREGISTRED`。

---

## 5. 実装すべきコマンド一覧

subject が **明示的に必須** としているのは以下。それ以外は reference client の要求に応じて。

| コマンド | 用途 | 主なエラー |
|---|---|---|
| `PASS` | パスワード認証 | 461, 462, 464 |
| `NICK` | ニック設定/変更 | 431, 432, 433, 436 |
| `USER` | ユーザー情報登録 | 461, 462 |
| `JOIN` | チャネル参加 | 403, 405, 471(+l), 473(+i), 474(+b), 475(+k) |
| `PART` | チャネル離脱 | 442, 403 |
| `PRIVMSG` | メッセージ送信 | 401, 403, 404(+n/+m), 407, 411, 412 |
| `NOTICE` | 通知(自動応答禁止) | — サーバは絶対に return しない |
| `QUIT` | 切断 | — |
| `PING` / `PONG` | 生存確認 | 409 |
| `KICK`(op) | メンバー追放 | 403, 442, 461, 482, 441 |
| `INVITE`(op) | 招待 | 401, 403, 442, 443 |
| `TOPIC` | トピック表示/変更 | 331, 332, 442, 482(+t 時) |
| `MODE`(user/channel) | モード変更 | 401, 403, 461, 467, 472, 482, 501 |

### コマンド構文と応答詳細

#### `PASS <password>`
- 登録前のみ。登録後は 462。
- パスワード無/不足は 461。**不一致は 464 → 切断** が慣例。

#### `NICK <nickname>`
- 461 なし(RFC 2812 は `NICK` 引数無しを **431 ERR_NONICKNAMEGIVEN**)
- 不正文字/長さ超過: 432
- 重複: 433(既存ニックと大文字小文字無視で一致)
- 変更成功時は `:<oldnick>!user@host NICK :<newnick>` を **本人 + 同じチャネルにいる全員** にブロードキャスト。

#### `USER <user> <mode> <unused> :<realname>`
- 4 パラ未満は 461。
- 登録済で再送は 462。
- `<mode>` と `<unused>` は無視して良い(2812 では `<mode>` はビットマスク、`<unused>` は `*` などが入る)。

#### `JOIN <channel>{,<channel>} [<key>{,<key>}]`
- 特殊: `JOIN 0` = **全チャネルから離脱**(PART と等価)。
- チャネル未存在なら **作成し、作成者を自動的に operator(+o)** にする。
- 参加チェックの順序(重要): `+k`(475) → `+i`(473) → `+l`(471) → `+b`(474)。
- 成功時の応答(1 チャネルあたり):
  1. `:nick!u@h JOIN :#chan` を **チャネル全員に** 送る
  2. RPL_TOPIC(332)/RPL_NOTOPIC(331)
  3. RPL_NAMREPLY(353) + RPL_ENDOFNAMES(366) を **本人にだけ**

#### `PART <channel>{,<channel>} [:<reason>]`
- 未参加は 442。
- 成功時: `:nick!u@h PART #chan :reason` を全員に送信、その後メンバーから外す。

#### `PRIVMSG <target>{,<target>} :<text>`
- target 無し: 411, text 無し: 412。
- 存在しない nick: 401, 存在しないチャネル: 403。
- `+n` の外部からの送信: 404 ERR_CANNOTSENDTOCHAN。
- `+m` で voice/op でない: 404。
- チャネル宛は **送信者以外の全メンバー** に転送。DM は相手 1 人に転送。
- `:` から始まる trailing で本文を扱う。長文は 512 制約に注意。

#### `NOTICE` — 意味は PRIVMSG と同じだが **サーバは応答/エラーを一切返してはならない**(loop 防止)。

#### `KICK <channel>{,<channel>} <user>{,<user>} [:<comment>]`
- 送信者が op でなければ 482。
- target が非メンバーは 441。
- 成功時 `:kicker!u@h KICK #chan target :comment` を全員に送信し、対象を退去させる。

#### `INVITE <nickname> <channel>`
- 送信者が非メンバーは 442。`+i` の場合 op でなければ 482。
- 対象が既にいれば 443。
- 成功: 招待リストに追加、`341 RPL_INVITING` を送信者に、`:inviter!u@h INVITE target :#chan` を対象に。

#### `TOPIC <channel> [:<topic>]`
- topic 省略なら現在値を返す(332 or 331)。
- topic あり: `+t` で op でなければ 482。空文字列(`TOPIC #ch :`)で topic クリア。
- 変更成功時は `:setter!u@h TOPIC #chan :new` を全員に送信。

#### `MODE`
- 対象が nickname → ユーザモード、`#`/`&` で始まる → チャネルモード。
- **課題必須のチャネルモード: `i`, `t`, `k`, `l`, `o`**。
- 複数モード同時指定可: `MODE #ch +itk-l password`。1 度に処理するモードは **3 つまで** に制限するのが慣例(RFC 2812 §3.2.3)。
- 成功時 `:setter MODE #chan +i-l+k pass` を全員にブロードキャスト。

##### 各チャネルモードの詳細
| mode | パラメータ | 動作 |
|---|---|---|
| `i` | なし | invite-only。INVITE か招待マスクに合致した者だけ JOIN 可 |
| `t` | なし | TOPIC 変更を operator に限定 |
| `k` | パスワード | JOIN 時に `JOIN #ch key` の一致を要求 |
| `l` | 整数 | 参加者上限。超過時 471 |
| `o` | nickname | 指定ユーザに operator 権限を付与/剥奪 |

- `+k` は既に鍵が設定されている状態で再設定しようとしたら `467 ERR_KEYSET`。
- `+o` の対象がチャネル非メンバーなら `441 ERR_USERNOTINCHANNEL`。
- 未知モード文字: `472 ERR_UNKNOWNMODE`。
- `-l`, `-k`, `-i`, `-t` はパラメータ不要。

#### `PING <token>` / `PONG <token>`
- クライアントから PING 来たら **同じ token** で PONG を返す。
- サーバー→クライアントの生存確認は任意だが、実装するとタイムアウト切断が可能に。

---

## 6. Numeric Reply 全体マップ(mandatory で使う可能性が高いもの)

### RPL(成功系)
| コード | 名前 | フォーマット例 |
|---|---|---|
| 001 | RPL_WELCOME | `:s 001 nick :Welcome to the IRC Network, nick!user@host` |
| 002 | RPL_YOURHOST | `:s 002 nick :Your host is s, running version X` |
| 003 | RPL_CREATED | `:s 003 nick :This server was created ...` |
| 004 | RPL_MYINFO | `:s 004 nick s ver umodes cmodes` |
| 221 | RPL_UMODEIS | ユーザモード表示 |
| 324 | RPL_CHANNELMODEIS | `:s 324 nick #chan +itk key` |
| 331 | RPL_NOTOPIC | `:s 331 nick #chan :No topic is set` |
| 332 | RPL_TOPIC | `:s 332 nick #chan :topic text` |
| 341 | RPL_INVITING | `:s 341 inviter invitee #chan` |
| 353 | RPL_NAMREPLY | `:s 353 nick = #chan :@op +voice user1 user2` |
| 366 | RPL_ENDOFNAMES | `:s 366 nick #chan :End of NAMES list` |
| 375/372/376 | MOTD | (任意) |

### ERR(エラー系)
| コード | 名前 | 発生条件 |
|---|---|---|
| 401 | ERR_NOSUCHNICK | 対象 nick なし |
| 403 | ERR_NOSUCHCHANNEL | 対象 channel なし |
| 404 | ERR_CANNOTSENDTOCHAN | 送信不可(+n/+m/ban) |
| 405 | ERR_TOOMANYCHANNELS | 参加チャネル数上限 |
| 407 | ERR_TOOMANYTARGETS | target 数上限 |
| 409 | ERR_NOORIGIN | PING に origin なし |
| 411 | ERR_NORECIPIENT | PRIVMSG/NOTICE に受信者なし |
| 412 | ERR_NOTEXTTOSEND | 送信テキストなし |
| 421 | ERR_UNKNOWNCOMMAND | 未知コマンド |
| 431 | ERR_NONICKNAMEGIVEN | NICK 引数なし |
| 432 | ERR_ERRONEUSNICKNAME | 不正な nick 形式 |
| 433 | ERR_NICKNAMEINUSE | nick 重複 |
| 441 | ERR_USERNOTINCHANNEL | KICK/MODE +o の対象が非メンバー |
| 442 | ERR_NOTONCHANNEL | 自分が非メンバー |
| 443 | ERR_USERONCHANNEL | INVITE 対象が既に参加中 |
| 451 | ERR_NOTREGISTERED | 未登録でコマンド送信 |
| 461 | ERR_NEEDMOREPARAMS | パラメータ不足 |
| 462 | ERR_ALREADYREGISTRED | 登録済で PASS/USER |
| 464 | ERR_PASSWDMISMATCH | PASS 不一致 |
| 467 | ERR_KEYSET | 既に +k セット済 |
| 471 | ERR_CHANNELISFULL | +l 上限 |
| 472 | ERR_UNKNOWNMODE | 未知モード文字 |
| 473 | ERR_INVITEONLYCHAN | +i でありながら未招待 |
| 474 | ERR_BANNEDFROMCHAN | ban 中 |
| 475 | ERR_BADCHANNELKEY | +k 鍵不一致 |
| 476 | ERR_BADCHANMASK | 不正な channel 名 |
| 481 | ERR_NOPRIVILEGES | IRC operator 権限なし |
| 482 | ERR_CHANOPRIVSNEEDED | チャネル op 権限なし |
| 501 | ERR_UMODEUNKNOWNFLAG | 未知ユーザモード |

**フォーマット規則:** `:<server> NNN <target> [<args>...] :<human readable msg>`
- 常に prefix にサーバー名。
- 数字は 3 桁ゼロ埋め(001, 042, ...)。
- 最初の param は必ず **クライアントの nick**(未登録なら `*`)。

---

## 7. Channel modes(RFC 2811 §4)

**課題必須は i, t, k, l, o の 5 つ**。他は実装しなくて良い(実装したければどうぞ)。

| flag | パラメータ | 種別 | 意味 |
|---|---|---|---|
| **i** | — | 課題必須 | invite-only |
| **t** | — | 課題必須 | topic 変更を op に制限 |
| **k** | key | 課題必須 | channel key(password) |
| **l** | limit | 課題必須 | user 数上限 |
| **o** | nick | 課題必須 | operator 権限 give/take |
| b | mask | (任意) | ban mask |
| e | mask | (任意) | ban exception |
| I | mask | (任意) | invite exception |
| m | — | (任意) | moderated |
| n | — | (任意) | no external msg |
| s | — | (任意) | secret |
| p | — | (任意) | private |
| v | nick | (任意) | voice |

**慣例(reference client が期待する動作):**
- 未実装モードが送られたら 472 でエラーにする。落とさず握りつぶすのはダメ。
- MODE 引数無しは現在の状態を返す(324 RPL_CHANNELMODEIS)。
- `+kl` のように パラメータを取るモードを複数指定した場合、指定順にパラメータを消費する。

---

## 8. 実装上の落とし穴

### 8.1 ノンブロッキング I/O
- `socket()` した後 `fcntl(fd, F_SETFL, O_NONBLOCK)` で non-blocking 化。
- listen socket も client socket も全て一つの `pollfd[]` 配列で管理し、**1 つの `poll()`** で監視する。
- `poll` の revents で判定:
  - `POLLIN`: `recv()` → 受信バッファに append → `\r\n` があれば行取り出してパース。
  - `POLLOUT`: 送信バッファが空でないときだけ有効化して `send()`。送信できた分だけ削除。部分送信は当たり前と考える。
  - `POLLHUP` / `POLLERR`: 切断処理。
- **絶対にやってはいけない:** `errno == EAGAIN` を見てリトライ。`send()` の戻りが小さくても即座に再送ループ。→ subject に「0 点」と明記。

### 8.2 メッセージの再構築
- TCP はストリームなので、1 回の `recv` で複数コマンドが来ることも、1 コマンドが分割されて届くこともある。
- 各 Client に **受信バッファ(std::string)** を持たせ、`recv` した内容を末尾に append。
- 先頭から `\r\n`(または `\n`)を探し、見つかった分だけ切り出してパースし、残りはバッファに残す。
- 512 byte 上限を超えたら切り捨てるか切断するか(実装者判断。落とさないこと)。

### 8.3 送信バッファ
- 各 Client に送信バッファも持たせ、応答は `client.appendSendBuffer()` で貯める。
- `poll` の POLLOUT で実際に `send()` する。部分送信ぶんだけ `erase()`。
- 送信バッファが空の間は `POLLOUT` を要求しないほうが CPU に優しい(必須ではない)。

### 8.4 大文字小文字の等価性
- **Nick / Channel の比較は必ず小文字化してから**。
- 変換規則(RFC 1459/2812):
  - `A..Z → a..z`
  - `{|}~ → []\^` の小文字扱い(2812)。1459 は `~` を含めない。
  - このリポジトリの `IrcCaseMapping.hpp` がその実装。

### 8.5 QUIT / 切断の伝播
- クライアントが `QUIT` を送るか、TCP が切れた場合:
  1. 参加している **全チャネル** に `:nick!u@h QUIT :reason` をブロードキャスト
  2. 各チャネルの members から除去
  3. 空になったチャネルは削除
  4. fd を close して pollfd 配列から外す
- `close(fd)` は必ず 1 回だけ。忘れると fd リーク。

### 8.6 SIGPIPE
- 相手切断済の fd に `write` すると SIGPIPE で落ちる。
- 対策:
  - `signal(SIGPIPE, SIG_IGN)` を main で 1 回呼ぶ、または
  - `send()` に `MSG_NOSIGNAL` フラグを付ける(Linux)
- 落ちたら subject 上 0 点になるので必須。

### 8.7 SIGINT / SIGTERM のハンドリング
- Ctrl-C で綺麗に落とすため、フラグを立てる signal handler を用意して event loop の条件にする。
- リソース(fd, Client, Channel)を全解放して exit。

---

## 9. テスト方法

### 9.1 nc による低レイヤ確認(subject が明示)
```bash
./ircserv 6667 pass
# 別ターミナル
nc -C 127.0.0.1 6667      # -C は CRLF 送信オプション
# CTRL-D で分割送信できる → パケット再結合の動作確認
PASS pass
NICK alice
USER alice 0 * :Alice
JOIN #test
PRIVMSG #test :hello
```

### 9.2 実クライアントで動作確認
reference client の候補:
- **irssi** (`sudo apt install irssi`) — 文字ベース、軽量。標準的な動作。
- **weechat** — irssi の高機能版。
- **HexChat** — GUI。ログが見やすい。
- **LimeChat** (Mac/Win) — GUI。

接続コマンド(irssi の例):
```
/connect 127.0.0.1 6667 pass
/nick alice
/join #test
/msg bob hi
```

### 9.3 動作確認シナリオ(peer eval で聞かれがち)
- 部分送信の再構築(nc で `com`^D`man`^D`d\n`)
- 2 クライアント間の PRIVMSG(相手にだけ届く)
- チャネル参加/離脱の JOIN/PART/QUIT の他ユーザーへのブロードキャスト
- `+i` で INVITE を挟まないと入れない
- `+k pass` で鍵が要る、`+l 2` で 3 人目が 471
- `+o` で権限 give/take
- op が KICK/TOPIC/MODE できる、非 op はできない(482)
- NICK 変更が同じチャネルの全員に見える
- 同一 nick で 2 人目が来たら 433
- パスワード違反は接続拒否
- 512 byte 超過メッセージの安全な扱い
- 1 クライアントが切断したときに他のセッションが影響を受けない

---

## 10. ディレクトリ構成(このリポジトリ)

```
ft_irc/
├── Makefile
├── include/
│   ├── commands/       # 各コマンドの hpp(Command パターン)
│   ├── core/           # Server / Dispatcher / Context
│   ├── domain/         # Client / Channel / Message
│   ├── response/       # ReplyBuilder / ResponseSink
│   └── utils/          # IrcCaseMapping / HostCaseMapping
├── src/
│   ├── app/main.cpp
│   ├── commands/       # 各コマンドの実装
│   ├── core/
│   ├── domain/
│   ├── response/
│   └── utils/
└── docs/
    └── IRC_KNOWLEDGE.md  ← このファイル
```

コマンドを 1 つ 1 つ `ACommand` を継承したクラスに分ける方針(Open/Closed Principle にも合致)。`CommandDispatcher` が `map<string, ACommand*>` で lookup。

---

## 11. 参考リンク

- RFC 1459 : https://datatracker.ietf.org/doc/html/rfc1459
- RFC 2811 (Channel Management) : https://datatracker.ietf.org/doc/rfc2811/
- RFC 2812 (Client Protocol) : https://datatracker.ietf.org/doc/html/rfc2812
- RFC 2813 (Server Protocol) : https://datatracker.ietf.org/doc/html/rfc2813 (この課題では **不要**)
- Modern IRC docs(現代の慣例) : https://modern.ircdocs.horse/
- Beej's Guide to Network Programming : https://beej.us/guide/bgnet/

---

## 付録: ちょっと詰まりやすいところ Q&A

**Q1. `USER` の第 2, 3 引数って何?**
RFC 2812 では `<mode>`(ユーザモードのビットマスク, 数値)と `<unused>`(通常 `*`)。ほとんどのサーバは無視して良い。RFC 1459 では `<hostname> <servername>` だがクライアントは信用されないのでこれも無視するのが今どき。

**Q2. NAMES 応答の `=` / `*` / `@` って何?**
`= public / * private / @ secret` の channel visibility 記号(2812 §5.1)。実装が面倒なら **常に `=`** で問題ない(reference client は気にしない)。

**Q3. `WHO` や `LIST`, `WHOIS` は必要?**
subject 上必須ではないが、reference client(特に irssi/HexChat)が接続直後にこれらを投げてくる。**未知コマンドとして無視するか 421 を返すか** で対処すれば OK(落ちなければ問題ない)。

**Q4. `CAP LS` が来るけど何?**
IRCv3 の機能ネゴシエーション。実装しない場合は `CAP * LS :` のような空応答を返すか、421 を返して無視すれば大抵のクライアントは登録シーケンスに進んでくれる。

**Q5. reply の nick は何を入れる?**
登録前は `*`。登録後はそのクライアントの現在の nick。

**Q6. `PART #a,#b` みたいなカンマ区切りは全部処理する?**
はい。1 つずつループしてそれぞれについて成功/失敗を返す。1 つ失敗しても他は続行。

**Q7. `poll()` のタイムアウトは?**
`-1` で無限待機で OK。定期処理(PING keepalive など)を入れたい場合のみ有限値。

**Q8. サーバ側から PING を送るタイミングは?**
mandatory では実装不要。実装するなら、最後の受信から 60s 経過で PING を送り、更に 60s PONG が来なければ切断、等。
