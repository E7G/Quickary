# Quickary Local HTTP API

Quickary 1.1 includes an **optional** localhost-only file search API for local integrations, scripts, launchers, editor plugins, and automation.

The API is disabled by default. Enable it in **Settings → API**.

## Security model

- Binds only to `127.0.0.1`.
- Search requests require a random bearer token generated locally by Quickary.
- Only `GET` is accepted.
- Request headers are capped at 16 KiB.
- Query strings are capped at 512 characters.
- Results are capped at 200 per request.
- At most 8 API searches may wait in the in-process queue.
- Responses use `Cache-Control: no-store` and close the connection after each response.
- The API never exposes a remote/LAN listening option.
- When disabled, the listener is closed and the API's dedicated Everything provider is not created.

The token can be rotated from **Settings → API → Regenerate token**.

## Health

`GET /health` does not require authentication.

Example response:

```json
{"ok":true,"service":"Quickary","version":"1.1.0"}
```

## Search

`GET /v1/search?q=<query>&limit=<1-200>`

Authentication can be sent using either:

```text
Authorization: Bearer <token>
```

or:

```text
X-Quickary-Token: <token>
```

PowerShell example:

```powershell
$token = '<token from Quickary Settings>'
$uri = 'http://127.0.0.1:32142/v1/search?q=report%20ext%3Apdf&limit=25'
Invoke-RestMethod -Uri $uri -Headers @{ Authorization = "Bearer $token" }
```

Example response:

```json
{
  "query": "report ext:pdf",
  "count": 2,
  "results": [
    {
      "type": "file",
      "name": "report.pdf",
      "path": "C:/Work/report.pdf",
      "size": 184320,
      "modified": "2026-08-23T09:45:00.000+08:00"
    }
  ]
}
```

The API intentionally exposes the Everything-backed file/folder search path only. It does not execute Quickary commands, web actions, shell actions, or arbitrary programs.

## Search syntax

The `q` parameter accepts the same Everything-oriented syntax used by Quickary file search, including filters such as `ext:`, `size:`, `date:`/`dm:`, `path:`, `parent:`, `regex:`, and explicit `content:` / `text:` queries when supported by the installed Everything version.
