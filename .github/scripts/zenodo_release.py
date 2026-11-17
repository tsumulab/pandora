#!/usr/bin/env python3
"""Archive a GitHub release as a new version of PANDORA's Zenodo record.

Uses Zenodo's records API (InvenioRDM). A new version starts as a copy of the
latest one's metadata (creators, ORCIDs, affiliations, funding, license, ...),
so only what changes per release is sent: version, publication date, a link to
the GitHub release, and the archive. Edit anything else on Zenodo itself; the
next release inherits it.

    zenodo_release.py ARCHIVE --version 1.2.0 --release-url URL [--publish]

Environment: ZENODO_TOKEN (scopes deposit:write, deposit:actions),
             ZENODO_RECORD (any version of the record; default 23908117),
             ZENODO_URL (default https://zenodo.org; https://sandbox.zenodo.org to test).
Without --publish the new version is left as a draft to be reviewed and
published on Zenodo.
"""
import argparse, datetime, json, os, sys, urllib.error, urllib.request

BASE = os.environ.get("ZENODO_URL", "https://zenodo.org").rstrip("/")
RECORD = os.environ.get("ZENODO_RECORD", "23908117")
TOKEN = os.environ.get("ZENODO_TOKEN", "")


def call(method, path, body=None, raw=None, ctype="application/json"):
    url = path if path.startswith("http") else BASE + path
    data = raw if raw is not None else (json.dumps(body).encode() if body is not None else None)
    req = urllib.request.Request(url, data=data, method=method)
    req.add_header("Authorization", "Bearer " + TOKEN)
    req.add_header("Accept", "application/json")
    if data is not None:
        req.add_header("Content-Type", ctype)
    try:
        with urllib.request.urlopen(req, timeout=600) as r:
            txt = r.read()
            return json.loads(txt) if txt else {}
    except urllib.error.HTTPError as e:
        sys.exit(f"{method} {url}: {e.code} {e.read().decode(errors='replace')[:2000]}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("archive")
    ap.add_argument("--version", required=True)
    ap.add_argument("--release-url", required=True)
    ap.add_argument("--publish", action="store_true")
    a = ap.parse_args()
    if not TOKEN:
        sys.exit("ZENODO_TOKEN is not set")

    latest = call("GET", f"/api/records/{RECORD}/versions/latest")
    print(f"latest version: {latest['id']} ({latest['metadata'].get('version')})")
    if latest["metadata"].get("version") == a.version:
        sys.exit(f"version {a.version} is already published on Zenodo")

    # New version (a draft). If one is already open, Zenodo returns it.
    draft = call("POST", f"/api/records/{latest['id']}/versions")
    rid = draft["id"]
    print(f"draft: {BASE}/uploads/{rid}")

    # Metadata: inherited; change only what is per-release.
    md = draft["metadata"]
    md["version"] = a.version
    md["publication_date"] = datetime.date.today().isoformat()
    md["dates"] = []
    rel = [r for r in md.get("related_identifiers", [])
           if "/releases/tag/" not in r.get("identifier", "")]
    rel.insert(0, {"identifier": a.release_url, "scheme": "url",
                   "relation_type": {"id": "issupplementto"},
                   "resource_type": {"id": "software"}})
    md["related_identifiers"] = rel
    body = {"metadata": md, "access": draft["access"], "files": {"enabled": True}}
    if draft.get("custom_fields"):
        body["custom_fields"] = draft["custom_fields"]
    res = call("PUT", f"/api/records/{rid}/draft", body)
    if res.get("errors"):
        print("metadata warnings:", json.dumps(res["errors"]))

    # Files: drop anything already in the draft, upload the archive.
    for e in call("GET", f"/api/records/{rid}/draft/files").get("entries", []):
        call("DELETE", f"/api/records/{rid}/draft/files/{e['key']}")
    key = os.path.basename(a.archive)
    call("POST", f"/api/records/{rid}/draft/files", [{"key": key}])
    with open(a.archive, "rb") as f:
        call("PUT", f"/api/records/{rid}/draft/files/{key}/content", raw=f.read(),
             ctype="application/octet-stream")
    call("POST", f"/api/records/{rid}/draft/files/{key}/commit")
    print(f"uploaded {key}")

    if a.publish:
        rec = call("POST", f"/api/records/{rid}/draft/actions/publish")
        print(f"published: https://doi.org/{rec['pids']['doi']['identifier']}")
    else:
        print(f"left as a draft; review and publish at {BASE}/uploads/{rid}")


if __name__ == "__main__":
    main()
