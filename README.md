![version](https://img.shields.io/badge/version-18%2B-EB8E5F)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-strophe)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-strophe/total)

# 4d-plugin-strophe

The XMPP plugin lets a 4D application send and receive [XMPP](https://xmpp.org/) (Jabber) messages by driving the `libstrophe` C library under the hood. It exposes two commands: one that opens a connection just long enough to send a single chat message, and one that opens a connection, listens for incoming messages for a short window, and returns whatever arrived. Both commands return a `Object`.

| Command | Returns | Purpose |
|---|---|---|
| [xmpp send message](#xmpp-send-message) | Object | Connect, send one XMPP chat stanza, disconnect |
| [xmpp connect](#xmpp-connect) | Object | Connect, listen for incoming messages for a short window, disconnect, return what was received |

**Platforms:** Windows, macOS

---

## Requirements & platform notes

- Both commands are synchronous and **block the calling process** until the underlying `libstrophe` connection finishes — either because it disconnected, timed out, or (for `xmpp connect`) went idle. Call from a preemptive process or a worker if you don't want the caller to stall.
- Both commands take their options in the same shape: a mandatory `Object` with `jid`, `password`, and `host` — there is no positional/simple-parameter form.
- **TLS certificate validation is off by default.** The plugin's `trustTLS` option defaults to `true`, which means it will accept an invalid or self-signed server certificate unless you explicitly pass `trustTLS: false`. The connection is encrypted by default (`mandatoryTLS` defaults to `true`), but not authenticated by default. If you're connecting to a server whose certificate you don't otherwise control/trust, set `trustTLS` to `false` explicitly.
- **The returned `log` collection defaults to full debug-level detail** (the `logLevel` option defaults to the most verbose level `libstrophe` supports). This can include low-level protocol detail from the authentication exchange. If you log or display `status.log` for troubleshooting, be aware of what you're persisting/displaying, and consider lowering `logLevel` outside of active debugging.
- Every option object field below is optional except `jid`, `password`, and `host`.

---

## xmpp send message

### Syntax

```
xmpp send message ( options : Object ; stanza : Object ) → Object
```

| Parameter | Type | Description |
|---|---|---|
| `options` | Object | Connection settings — see [Options object](#options-object) below. |
| `stanza` | Object | The message to send — see [Stanza object](#stanza-object) below. |
| Result | Object | Status object — see [Return value (xmpp send message)](#return-value-xmpp-send-message) below. |

#### Options object

| Property | Type | Description |
|---|---|---|
| `jid` | Text | Your XMPP JID (e.g. `"jimmy.fallon@localhost/m"`). |
| `password` | Text | Password for `jid`. |
| `host` | Text | XMPP server host to connect to. |
| `timeout` | Number | Overall connection timeout in seconds. Defaults to `20` if omitted or ≤ 0. |
| `logLevel` | Number | `libstrophe` log verbosity, `0`–`3` (DEBUG…ERROR). Any value outside `-1`–`3` is reset to `0` (DEBUG). Defaults to `0` (DEBUG) if omitted. |
| `keepAlive` | Boolean | Enables TCP keepalive. Defaults to `false`. |
| `keepAliveTimeout` | Number | Keepalive timeout in seconds. Any value > 0 also forces `keepAlive` on. Defaults to `60`. |
| `keepAliveInterval` | Number | Keepalive probe interval in seconds. Any value > 0 also forces `keepAlive` on. Defaults to `1`. |
| `disableTLS` | Boolean | Disables TLS entirely (plaintext connection). Defaults to `false`. When `true`, `mandatoryTLS`/`legacyTLS`/`trustTLS` are ignored. |
| `mandatoryTLS` | Boolean | Requires TLS for the connection to succeed. Defaults to `true`. Ignored if `disableTLS` is `true`. |
| `legacyTLS` | Boolean | Uses implicit/legacy SSL (TLS from the start of the connection, no `STARTTLS` negotiation). Defaults to `false`. Ignored if `disableTLS` is `true`. |
| `trustTLS` | Boolean | Accepts the server's TLS certificate even if it's invalid or untrusted. **Defaults to `true`** — see the caveat above. Ignored if `disableTLS` is `true`. |
| `enableLegacyAuth` | Boolean | Enables legacy (non-SASL) authentication. Defaults to `false`. |

#### Stanza object

Any string-valued property becomes an attribute of the outgoing `<message>` element, **except** `body`, which becomes the message's text content. Non-string properties (numbers, booleans, sub-objects, collections) are silently skipped — only string fields are sent.

| Property | Type | Description |
|---|---|---|
| `body` | Text | The message text. Becomes the `<body>` element's content. |
| `to` | Text | Recipient JID. Sent as the `to` attribute if present. |
| `type` | Text | Message type (e.g. `"chat"`). Sent as the `type` attribute if present. |
| `id` | Text | Stanza ID. Sent as the `id` attribute if present. Not auto-generated — supply your own (e.g. via `Generate UUID`) if you want one. |
| *(any other property)* | Text | Sent verbatim as an attribute of the same name. |

#### Return value (xmpp send message)

| Property | Type | Description |
|---|---|---|
| `log` | Collection | Collection of `{area; message}` objects captured from `libstrophe`'s internal logging, at whatever level `logLevel` specified. |

### Description

Opens a fresh connection with the given `options`, sends exactly one stanza built from `stanza`, then disconnects immediately — this command doesn't wait for a reply. There's no confirmation of delivery beyond the `log` collection; if you need a reply (e.g. an ack or an error stanza), use [xmpp connect](#xmpp-connect) in a follow-up call instead.

The outgoing stanza is always a `<message>` element of namespace `jabber:client`, in the sender's own JID's `from` (set automatically — you don't provide it). `xml:lang` is always set to `"en"`.

### Example

From the plugin's own test method (`TEST_send.4dm`):

```4d
//%attributes = {}
$params:=New object:C1471

$params.jid:="jimmy.fallon@localhost/m"
$params.password:="nbc"
$params.host:="localhost"

  //message stanza
$message:=New object:C1471(\
"body";"Hello!";\
"type";"chat";\
"id";Generate UUID:C1066;\
"to";"jimmy.kimmel@localhost")

$status:=xmpp send message ($params;$message)
```

Sending without TLS certificate trust, against a server whose certificate you know is valid:

```4d
$params:=New object:C1471
$params.jid:="alice@example.com/desktop"
$params.password:="secret"
$params.host:="xmpp.example.com"
$params.trustTLS:=False

$message:=New object:C1471("body";"Meeting moved to 3pm";"type";"chat";"to";"bob@example.com")

$status:=xmpp send message ($params;$message)
```

Checking the returned log:

```4d
$params:=New object:C1471
$params.jid:="alice@example.com/desktop"
$params.password:="secret"
$params.host:="xmpp.example.com"

$message:=New object:C1471("body";"ping";"type";"chat";"to";"bob@example.com")

$status:=xmpp send message ($params;$message)

For each ($entry;$status.log)
	ALERT($entry.area+": "+$entry.message)
End for each
```

---

## xmpp connect

### Syntax

```
xmpp connect ( options : Object ) → Object
```

| Parameter | Type | Description |
|---|---|---|
| `options` | Object | Connection settings — same shape as [xmpp send message](#options-object)'s `options` parameter. |
| Result | Object | Status object — see [Return value (xmpp connect)](#return-value-xmpp-connect) below. |

#### Return value (xmpp connect)

| Property | Type | Description |
|---|---|---|
| `log` | Collection | Same as [xmpp send message](#return-value-xmpp-send-message)'s `log`. |
| `messages` | Collection | Collection of received message objects — see below. Empty if nothing arrived before disconnect. |

Each element of `messages` is an object:

| Property | Type | Description |
|---|---|---|
| `body` | Text | The message's text content. |
| `type` | Text | The message's `type` attribute. |
| `name` | Text | The stanza element name (normally `"message"`). |
| `from` | Text | Sender JID. |
| `to` | Text | Recipient JID. |
| `id` | Text | Stanza ID, if the sender set one. |
| `ns` | Text | Stanza namespace. |

### Description

Opens a connection with the given `options` and stays connected, collecting incoming messages into `messages`, until one of three things happens: no message arrives for about 100 ms (the connection is considered idle and is closed), the overall `timeout` (default 20 seconds) elapses, or the connection itself drops.

**A message is only added to `messages` if it has both a `<body>` and an explicit `type` attribute.** Messages with no body are ignored outright; messages that omit the `type` attribute entirely are also dropped, even if they have a body — this includes some servers' own generated notices, which don't always set `type`. If you're not seeing an incoming message you expect, check whether the sender set `type`.

The plugin automatically answers `jabber:iq:version` queries (a standard XMPP "what software are you running" query) while connected, identifying itself as `libstrophe` version `0.9.3` — this is hardcoded and not configurable through `options`.

Because the idle window is short (~100 ms), this command is best suited to "check what's waiting right now" rather than holding a long-lived listening connection — for the latter, call it repeatedly (e.g. on a timer) rather than expecting one call to stay open indefinitely.

### Example

From the plugin's own test method (`TEST_receive.4dm`):

```4d
//%attributes = {}
$params:=New object:C1471

$params.jid:="jimmy.kimmel@localhost/m"
$params.password:="abc"
$params.host:="localhost"

$status:=xmpp connect ($params)
```

Polling for messages and printing each one:

```4d
$params:=New object:C1471
$params.jid:="jimmy.kimmel@localhost/m"
$params.password:="abc"
$params.host:="localhost"
$params.timeout:=10

$status:=xmpp connect ($params)

For each ($message;$status.messages)
	ALERT("From "+$message.from+": "+$message.body)
End for each
```

Giving the connection more room before it's considered idle isn't configurable — but you can widen the overall session with `timeout` if messages tend to arrive in bursts a few seconds apart:

```4d
$params:=New object:C1471
$params.jid:="jimmy.kimmel@localhost/m"
$params.password:="abc"
$params.host:="localhost"
$params.timeout:=60

$status:=xmpp connect ($params)
```

---

## Error handling & troubleshooting

- **No 4D-level error is raised on failure.** Both commands report everything — connection failures, authentication failures, TLS problems — only through the `log` collection in the returned object; check `status.log` rather than expecting a 4D method error.
- **`trustTLS` defaults to `true`.** By default the plugin will connect even if the server presents an invalid or self-signed certificate. Set it to `false` if you need the connection to fail on a bad certificate.
- **A message with no `type` attribute is silently dropped by `xmpp connect`**, even if it has a body. This is a common source of "I know a message was sent but it's not showing up in `messages`."
- **`xmpp connect` disconnects after ~100 ms of no new messages.** If you expect messages to trickle in slowly, don't assume one call will wait for all of them — raise `timeout` and/or call the command again.
- **Both commands block the calling process** for up to `timeout` seconds. Run them from a background/worker process if the caller can't afford to stall.
- **The returned `log` can contain verbose, low-level protocol detail** by default (`logLevel` defaults to the most detailed level). Be mindful of this if you persist or display it.
- **`stanza`'s non-string fields are silently ignored** by `xmpp send message` — only string-valued properties on the `stanza` object are sent. A number or boolean placed there won't produce an error, it just won't appear on the wire.

---

## Quick reference

```4d
// Send
$params:=New object:C1471
$params.jid:="me@example.com/app"
$params.password:="secret"
$params.host:="xmpp.example.com"

$message:=New object:C1471("body";"Hello";"type";"chat";"to";"them@example.com")
$status:=xmpp send message ($params;$message)

// Receive
$params:=New object:C1471
$params.jid:="me@example.com/app"
$params.password:="secret"
$params.host:="xmpp.example.com"
$params.timeout:=30

$status:=xmpp connect ($params)
For each ($message;$status.messages)
	ALERT($message.from+": "+$message.body)
End for each
```
