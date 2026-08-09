# IRC 概論 — プロトコルとしての IRC を理解する

このドキュメントは「IRC ってそもそも何なのか」を理解するためのもの。
実装の話は `IRC_KNOWLEDGE.md` に分けている。ここではプロトコル・仕組み・思想を扱う。

出典: RFC 1459 (原典), RFC 2810–2813 (改訂版), modern.ircdocs.horse (現代の慣例)

---

## 1. IRC とは何か

**IRC = Internet Relay Chat**。1988 年にフィンランドの Jarkko Oikarinen が作った、
インターネット上の **テキストベース・リアルタイム多人数チャット** のためのプロトコル。

- テキストのみ(画像・音声などは元来対象外)
- サーバとクライアントの間で **平文の 1 行コマンド** をやり取りする
- 「チャネル」に集まって多人数で話す形式が中心。1 対 1 の私信もできる
- 2024 年時点でも Libera.Chat, OFTC, EFnet, IRCnet などのネットワークが稼働中
- Slack, Discord, Matrix などの原型で、それらより **はるかにシンプル**

### なぜ今でも学ぶ価値があるのか

- **プロトコルが極めてシンプル**。TCP の上に「1 行 = 1 コマンド」を流すだけ。
  telnet や nc で手打ちできる。
- **応用範囲が広い設計思想**: パブサブ(publish-subscribe), 分散システムの
  ネットワーク分割問題, 状態同期, 権限モデル、といった話が全部詰まっている。
- **HTTP と対照的**: HTTP は 1 リクエスト = 1 レスポンスだが、IRC は
  **双方向・非同期・接続維持型**。WebSocket や chat 系プロトコルの直感が得られる。

---

## 2. アーキテクチャの全体像

```
                    IRC Network
                        │
      ┌─────────────────┼─────────────────┐
      │                 │                 │
   Server A ────────  Server B  ────── Server C     ← サーバー同士は
      │                 │                 │           spanning tree
   ┌──┴──┐           ┌──┴──┐           ┌──┴──┐       (ループのない木)
  Cli   Cli         Cli   Cli         Cli   Cli
```

### 3 つの主体

| 主体 | 役割 |
|---|---|
| **Client** | ユーザーが使うプログラム(irssi, HexChat, weechat, LimeChat 等)。1 人の人間に紐づく |
| **Server** | メッセージを中継するプログラム(ircd)。クライアントを受け入れ、他サーバに配信する |
| **Service** | サーバに常駐する特殊なプログラム(NickServ, ChanServ 等)。実質 bot |

### ネットワークとは

複数の Server が互いに接続して 1 つの **論理的なチャット空間** を作ったもの。
Libera.Chat, EFnet, IRCnet 等が有名なネットワーク。
**同じネットワークに属するサーバ間ではチャネルと nickname が共有される。**

- あるサーバに `#linux` に人が居れば、他サーバの `#linux` は同じチャネル
- あるサーバで `alice` が居れば、他サーバは `alice` を使えない

### spanning tree(木構造)

- サーバ同士は必ず **木構造** で接続する(ループ禁止)。
- 1 メッセージはツリー上を最短経路で 1 度だけ流れる。
- サーバ間リンクが切れると **network split**(通称 netsplit)が発生。
  同じ nick を持つ人が両側にできる、といった歴史的問題の温床。

**42 の ft_irc では「server-to-server 通信は実装しない」ので、
考えるべきは常に「1 台のサーバに複数クライアント」の構造のみ。**

---

## 3. 通信の基本 — 1 行 1 コマンド

IRC の通信は極めて素朴。TCP 接続を張ったら、以下の形の **テキスト行** を送り合う。

```
[':' <prefix> ' '] <command> [<params>] '\r\n'
```

- **全部 ASCII テキスト**
- **1 行が 1 コマンド**。区切りは CR-LF (`\r\n`)
- **1 行あたり最大 512 byte**(CR-LF 含む)
- **バイナリなし、フレーミングなし**。だから telnet や nc で手打ちできる

### 手打ちの例(telnet で Libera.Chat に接続する想定)

```
$ telnet irc.libera.chat 6667

NICK alice_test
USER alice_test 0 * :Alice Test
                                    ← ここでサーバから 001-004 の歓迎メッセージ
JOIN #test
                                    ← JOIN 通知と NAMES 応答
PRIVMSG #test :hello, world
                                    ← 他の参加者に届く
QUIT :bye
```

**IRC は本質的にこれだけ**。あとは全部この上に積んでいるだけ。

### メッセージの構造要素

| 要素 | 役割 | 例 |
|---|---|---|
| **prefix** | 送信元(通常サーバが付ける) | `:alice!user@host` or `:irc.example.net` |
| **command** | コマンド名または 3 桁数字 | `PRIVMSG` / `001` |
| **params** | 引数 (最大 15 個) | `#test` |
| **trailing** | `:` で始まる最後の引数(スペース可) | `:hello, world` |

`:` で始まる引数がなぜ特別かというと、**スペースを含んで良い引数** が必要だから。
中の空白で切られたら困る本文(メッセージ, real name, quit reason 等)は
全部 trailing に置く。

---

## 4. 3 つの主要概念: User / Channel / Message

### 4.1 User(ユーザー)

IRC の「人」を表すのは以下の 3 つの識別子:

| 名前 | 意味 | 変更可否 |
|---|---|---|
| **nickname** | チャット上の表示名。同ネットワーク内でユニーク | 可(NICK コマンド) |
| **username / ident** | ログイン名(接続時の USER で申告) | 不可(登録後) |
| **realname** | 本名や自己紹介文(表示のみ) | 不可(登録後) |

これらとホストを組み合わせた **prefix**(hostmask とも呼ぶ)が
メッセージの「送信元」として使われる:

```
nickname!username@hostname
例: alice!aliceuser@192-168-1-42.example.net
```

**大事な点:**
- IRC は「アカウント制」ではない。nick は誰でも自由に名乗れる(先勝ちで空いていれば)
- 恒常的なアカウントが欲しい場合は NickServ(サービス)で登録する文化がある
- したがって **認証は基本的に弱い**。「その nick を騙る誰か」の可能性は常にある

### 4.2 Channel(チャネル)

- **チャット部屋**。名前は `#linux` のように `#` で始まる
- ユーザが JOIN してメンバーになり、PART で抜ける
- 誰かがチャネルに送ったメッセージは、他の全メンバーに配信される
- 最初にその名前で JOIN した人が作った瞬間にチャネルが誕生し、
  最後の人が去った瞬間にチャネルが消える(暗黙的作成・消滅)

#### チャネル名のプレフィックス
| prefix | 種類 | 特徴 |
|---|---|---|
| `#` | 通常 | ネットワーク全体で共有。最も一般的 |
| `&` | ローカル | そのサーバに繋いだ人だけが見える |
| `+` | modeless | モードなし、operator 概念なし |
| `!` | safe | netsplit 対策で ID を持つ |

現代のクライアントは基本的に `#` しか使わない。他は歴史遺物寄り。

#### Channel Operator(通称 op, chanop)

チャネルの管理者権限を持つメンバー。以下ができる:

- **KICK** : メンバーを追放する
- **INVITE** : invite-only チャネルへの招待
- **TOPIC** : (制限モード時) トピック変更
- **MODE** : チャネル自体の設定変更、他人への op 権限付与

**表示上の記号**: NAMES や WHO の応答で nick の前に `@` が付く(`@alice`)。
voice 権限持ちは `+` が付く(`+bob`)。

**チャネル作成者は自動的に op になる**。他人に op を渡すのは MODE +o コマンド。

#### 発言権(voice)

- `+m` (moderated) モードだと、op か **+v(voice)** を持つ人しか発言できない
- 「読める人は多いが発言者を限定したい」場面(講演形式など)で使う

### 4.3 Message(メッセージ)

IRC で流れる「情報の単位」は 4 種類くらいある:

| 種類 | コマンド | 用途 |
|---|---|---|
| チャネルへの発言 | `PRIVMSG #chan :text` | チャネル全員に配信 |
| DM (private message) | `PRIVMSG nick :text` | 特定 1 人に配信 |
| 通知 (notice) | `NOTICE target :text` | PRIVMSG に似るが **自動応答してはいけない** |
| CTCP | `PRIVMSG target :\x01ACTION waves\x01` | 特殊なメタメッセージ (/me, /version 等) |

**PRIVMSG と NOTICE の使い分け:**
- クライアント同士の普通の会話 → PRIVMSG
- サーバや bot からの「エラー・通知」 → NOTICE
- NOTICE に対して自動応答を返すのは禁止(bot ループ防止)

**CTCP (Client-To-Client Protocol):**
- 本文を `\x01` (ASCII 1) で挟むと特殊メタメッセージ扱い
- `ACTION` は `/me waves` の実装 → `\x01ACTION waves\x01`
- `VERSION`, `TIME`, `PING`, `DCC` などがある
- **DCC (Direct Client-to-Client)** はサーバを介さず直接クライアント同士で
  ファイル転送や音声チャットするための仕組み

---

## 5. 接続から会話までの流れ

新しいクライアントがサーバに繋いだ後の全体シーケンス。

```
   Client                            Server
     │                                 │
     │──── TCP connect (port 6667) ───▶│
     │                                 │
     │──── PASS <password> ───────────▶│  (任意)
     │──── NICK <nick> ───────────────▶│
     │──── USER <u> <m> <*> :<r> ─────▶│
     │                                 │
     │◀─── 001 RPL_WELCOME ────────────│  「ようこそ」
     │◀─── 002 RPL_YOURHOST ───────────│
     │◀─── 003 RPL_CREATED ────────────│
     │◀─── 004 RPL_MYINFO ─────────────│
     │◀─── 375/372/376 MOTD ───────────│  (任意)
     │                                 │ ─── ここから会話フェーズ ───
     │                                 │
     │──── JOIN #foo ─────────────────▶│
     │◀─── :self JOIN :#foo ───────────│  自分にも JOIN 通知
     │◀─── 332/331 TOPIC ──────────────│
     │◀─── 353 NAMES ──────────────────│  現在のメンバー
     │◀─── 366 END OF NAMES ───────────│
     │                                 │
     │──── PRIVMSG #foo :hi ──────────▶│
     │                                 │──▶ (他メンバー全員へ配信)
     │                                 │
     │──── QUIT :bye ─────────────────▶│
     │                                 │──▶ (関係チャネルに通知)
     │◀─── ERROR :Closing link ────────│
     │                                 │
     │◀────────── TCP close ──────────▶│
```

### 登録フェーズ (registration)

TCP を繋いだ直後、クライアントは **PASS → NICK → USER** の順にコマンドを送る。
サーバはこの 3 つが揃うまで(パスワード不要な場合は NICK と USER の 2 つ)、
それ以外のコマンドを受け付けてはいけない (`451 ERR_NOTREGISTERED`)。

登録が完了すると、サーバは **001〜004** の 4 つの welcome numeric を送って、
「会話していいよ」と告げる。この 001 が来たかどうかがクライアント側の
「接続完了」判定の指標になっている。

### 会話フェーズ

以降は **完全に非同期**。クライアントはいつでもコマンドを送れるし、
サーバはいつでもメッセージを流し込んでくる。「リクエスト・レスポンス」の
概念は薄く、**イベントベース** で動く。

---

## 6. コマンドの体系

RFC 2812 では 40 種類以上のコマンドが定義されている。分類して眺めると理解しやすい。

### 6.1 接続管理
- `PASS` — パスワード認証
- `NICK` — ニックネーム設定/変更
- `USER` — ユーザー情報登録
- `QUIT` — 切断
- `PING` / `PONG` — 生存確認

### 6.2 チャネル操作
- `JOIN` — 参加
- `PART` — 離脱
- `TOPIC` — トピック表示/変更
- `NAMES` — メンバー一覧
- `LIST` — サーバ上のチャネル一覧
- `INVITE` — 招待
- `KICK` — 追放
- `MODE` — チャネルモード変更 (i, t, k, l, o, b, v, m, n, s, p ...)

### 6.3 メッセージ送信
- `PRIVMSG` — 通常メッセージ
- `NOTICE` — 通知(自動応答禁止)

### 6.4 情報取得
- `WHO` — ユーザー検索
- `WHOIS` — 特定 nick の詳細
- `WHOWAS` — 過去に居た nick の情報
- `MOTD` — Message Of The Day
- `LUSERS` — サーバ統計
- `VERSION`, `TIME`, `INFO`, `STATS` — サーバ情報

### 6.5 サーバ運用者向け
- `OPER` — IRC operator に昇格(サーバ管理者権限。チャネル op と別物)
- `KILL` — ネットワークからの強制切断
- `REHASH`, `RESTART`, `DIE` — サーバ制御
- `WALLOPS` — 全 operator への一斉通知

### 6.6 サーバ間通信 (server-to-server)
- `SERVER`, `SQUIT`, `NJOIN` など。ft_irc では実装不要。

---

## 7. 応答の体系 — Numeric Reply

サーバから返る応答には **3 桁の数字コード** が付いている。
HTTP の 200/404 のような統一 API。

### コードの読み方

- **001–099**: クライアント/サーバ間の接続関連
- **200–399**: 情報応答 (RPL_*)
- **400–599**: エラー応答 (ERR_*)

### 形式

```
:<server> <NNN> <target> [<args>...] :<human readable message>
```

- 常に **サーバ prefix** から始まる
- 3 桁数字がコマンド代わり
- 第 1 引数は **常に受信者の nick**(未登録時は `*`)
- 最後は必ず人間可読な説明文

### よく出るコード早見表

| 番号 | 名前 | 意味 |
|---|---|---|
| 001 | RPL_WELCOME | 接続完了 |
| 331 | RPL_NOTOPIC | トピック未設定 |
| 332 | RPL_TOPIC | トピック表示 |
| 353 | RPL_NAMREPLY | チャネルメンバー一覧 |
| 366 | RPL_ENDOFNAMES | メンバー一覧終了 |
| 401 | ERR_NOSUCHNICK | 相手が居ない |
| 403 | ERR_NOSUCHCHANNEL | チャネルが無い |
| 433 | ERR_NICKNAMEINUSE | nick 重複 |
| 451 | ERR_NOTREGISTERED | 登録前にコマンド |
| 461 | ERR_NEEDMOREPARAMS | 引数不足 |
| 464 | ERR_PASSWDMISMATCH | パスワード違反 |
| 482 | ERR_CHANOPRIVSNEEDED | op 権限不足 |

### 何が数字応答で、何がテキスト応答か

- **サーバがクライアントに返す** ものは 数字応答
- **他クライアントの行動を伝える** ものはコマンド名の応答
  - 例: alice が JOIN すると、bob には `:alice!u@h JOIN :#chan` が届く

これ、直感的でなくてよく混乱する。イメージ:

| 種類 | 例 | 発生 |
|---|---|---|
| 数字応答 | `:server 332 alice #chan :topic` | サーバがあなたに情報を返す |
| コマンド応答 | `:bob!u@h PART #chan :bye` | 他人の行動があなたに通知される |

---

## 8. 大文字小文字の扱い(意外な落とし穴)

IRC は「Alice と alice は同じ人」として扱う。**大文字小文字を区別しない**。

しかも普通の ASCII の toupper/tolower ではない。**Scandinavian rule** という
歴史的経緯のマッピングがある。

| 大文字 | 小文字 |
|---|---|
| A–Z | a–z |
| `[` | `{` |
| `]` | `}` |
| `\` | `|` |
| `^` | `~` |  (RFC 2812 のみ)

理由: 昔のスカンジナビア諸国のキーボード配列で、`{}|` は `[]\` の異体字扱いだった。
文字コード上も `[` (0x5B) → `{` (0x7B) のように大文字小文字関係にある。
IRC はフィンランド発祥のため、この慣習を取り込んだ。

**チャネル名にも同じ規則が適用される**。`#Linux` と `#LINUX` は同じチャネル。

---

## 9. 特筆すべき設計上のクセ

### 9.1 認証が弱い

- 素の IRC には「ログイン」の概念がない
- USER で申告する ident (username) は自己申告で、サーバは検証しない
  (昔は ident プロトコル (RFC 1413) で確認したが今はほぼ廃れた)
- 「本物の alice」を保証したいなら **サービス層** (NickServ) が必要
- 現代の実装では SASL 認証 (AUTHENTICATE) を使うのが標準

### 9.2 状態が完全に揮発

- チャネルは「今いる人がいる限り存在する」
- 誰もいなくなったら **チャネル設定もトピックも全部消える**
- 常設チャネルにしたければ ChanServ でチャネルを「登録」する必要がある
- Slack/Discord から来ると驚くが、これは仕様

### 9.3 メッセージは記録されない

- サーバはログを取らない(取るのは各クライアント)
- 「後から入った人は前の会話が見えない」のが基本
- ScrollBack / offline message は近年の IRCv3 拡張で対応中

### 9.4 プッシュ配信

- サーバはクライアントの状態を **能動的に押し込む**
- クライアントは常に接続を維持し、いつ来るかわからないメッセージを待つ
- HTTP のように「聞くまで返さない」ではない

### 9.5 512 byte の呪縛

- 1 メッセージ 512 byte(CR-LF 含む)の上限がある
- 長いメッセージ、複数言語(UTF-8)、絵文字などは分割される可能性がある
- 現代の拡張(message tags 等)でこの制限を回避しつつある

---

## 10. IRC のセキュリティモデル

### 権限の 2 段構え

| 種類 | 呼称 | 権限範囲 |
|---|---|---|
| サーバ全体 | **IRC operator** (通称 IRCop, oper) | サーバの管理者権限。KILL, REHASH, DIE 等 |
| チャネル単位 | **channel operator** (通称 chanop, op) | そのチャネルの管理。KICK, MODE, TOPIC 等 |

この 2 つは **完全に別物**。よく混同される。
`@` 記号が付くのは chanop のほう。

### アクセス制御(チャネル単位)

- `+i` invite-only : INVITE されないと入れない
- `+k` key : パスワード必要
- `+l` limit : 人数上限
- `+b` ban : 特定 hostmask を追放
- `+e` ban exception : ban に対する例外
- `+I` invite exception : invite-only に対する例外

### hostmask によるパターンマッチ

ban やアクセス制御では **hostmask** を使う:

```
nick!user@host
```

各要素で `*` (任意数の任意文字) と `?` (1 文字) が使える:

- `*!*@*.spam.example` — spam.example ドメイン全体を ban
- `bad*!*@*` — bad で始まる nick を全部 ban
- `*!*mallory@*` — mallory という ident を持つ人を全部 ban

---

## 11. 現代の IRC — IRCv3

RFC 2812 の後、IRC は正式な RFC 更新が長らく行われなかった。
その間、有志の集まり **IRCv3 Working Group** (ircv3.net) が拡張を進めている。

代表的な拡張:

| 名前 | 内容 |
|---|---|
| **CAP negotiation** | 拡張機能のネゴシエーション |
| **SASL** | 標準的な認証(AUTHENTICATE PLAIN / EXTERNAL 等) |
| **message-tags** | メッセージにメタデータを付加(時刻, ID 等) |
| **server-time** | 過去メッセージの正確な時刻表示 |
| **echo-message** | 自分の送信を自分にもエコー |
| **chghost** | ホスト名変更通知 |
| **account-tag** | 発言者のアカウント名タグ |

**ft_irc でこれらを実装する必要は基本ない** が、reference client が
`CAP LS` を送ってくるので、`CAP * LS :` の空応答か、421 で無視するかの対応は必要。

---

## 12. 主要なクライアント・サーバ実装

### クライアント (referenceに選ぶ候補)

| 名前 | プラットフォーム | 特徴 |
|---|---|---|
| **irssi** | Terminal (Unix) | 軽量・スクリプタブル。開発者に人気 |
| **weechat** | Terminal (Unix) | irssi の高機能版 |
| **HexChat** | GUI (cross-platform) | 初心者にも使いやすい。ログが見やすい |
| **LimeChat** | GUI (Mac/Windows) | シンプル |
| **Konversation** | GUI (KDE) | KDE 統合 |
| **The Lounge** | Web | サーバ側常駐 + ブラウザから使う |

### サーバ (ircd)

| 名前 | 特徴 |
|---|---|
| **InspIRCd** | モジュラー設計。実装リファレンスによく使われる |
| **UnrealIRCd** | 機能豊富 |
| **charybdis** | Libera.Chat で使われている |
| **ircd-hybrid** | EFnet 系 |
| **ngircd** | 軽量。ソースが読みやすい |
| **solanum** | charybdis のフォーク、現行 Libera.Chat |

**参考にするなら ngircd が読みやすい**(C, ソース小さい)。

---

## 13. IRC 文化のキーワード集

読み書きで頻出する用語。

| 用語 | 意味 |
|---|---|
| **lurker** | チャネルに居るが発言しない人 |
| **AFK** | Away From Keyboard(離席中) |
| **op** / **chanop** | channel operator |
| **oper** / **IRCop** | IRC operator (サーバ管理者) |
| **netsplit** | サーバ間リンク断絶。人が突然一斉に離脱する現象 |
| **flood** | 短時間大量のメッセージ送信。防止機構(flood protection)がある |
| **ban evasion** | ban を逃れる行為 |
| **kickban** | KICK と +b を同時にする定型操作 |
| **/me** | ACTION の慣用表記 (`/me waves` → `* alice waves`) |
| **hostmask** | `nick!user@host` の識別子 |
| **znc / bnc** | IRC bouncer。常時接続を代行するサーバサイドクライアント |
| **motd** | サーバに接続した時に表示されるメッセージ |
| **cloak** | 実 IP を隠すために表示される仮想ホスト |

---

## 14. IRC を「実際に触ってみる」

理解を深めるには実際に既存の IRC ネットワークに繋いでみるのが早い。

### telnet で生プロトコルを見る

```
$ telnet irc.libera.chat 6667

NICK ft_irc_test_42
USER ft_irc_test 0 * :Testing
```

サーバから返ってくるテキストを眺めると、ここまでの説明が全部生で見える。

### クライアントで参加

```bash
# irssi の場合
$ irssi
/connect irc.libera.chat
/join #libera
/msg NickServ HELP
```

Libera.Chat, OFTC, IRCnet あたりは今も活発。#libera, #ubuntu-jp,
#archlinux などが有名。

---

## 15. まとめ — IRC の本質を 5 行で

1. **TCP の上で 1 行 = 1 コマンドの平文を送り合うだけ**
2. **client-server 型、複数サーバがツリーで繋がって 1 ネットワーク**
3. **channel は暗黙的に作成/消滅する軽い pub-sub 部屋**
4. **サーバは状態(誰がどこに居るか)を持ち、変化を関係者に push する**
5. **数字コード応答とテキストコマンド応答、2 種類のメッセージが流れる**

これだけ理解していれば、あとは RFC を辞書代わりに引きながら実装や運用に進める。

---

## 参考リンク

- **RFC 1459** (原典・1993) : https://datatracker.ietf.org/doc/html/rfc1459
- **RFC 2810** (アーキテクチャ) : https://datatracker.ietf.org/doc/html/rfc2810
- **RFC 2811** (チャネル管理) : https://datatracker.ietf.org/doc/html/rfc2811
- **RFC 2812** (クライアントプロトコル) : https://datatracker.ietf.org/doc/html/rfc2812
- **RFC 2813** (サーバプロトコル) : https://datatracker.ietf.org/doc/html/rfc2813
- **modern.ircdocs.horse** (現代の慣例まとめ) : https://modern.ircdocs.horse/
- **IRCv3** (現代の拡張) : https://ircv3.net/
- **Libera.Chat** (見学するのに良い実ネットワーク) : https://libera.chat/
