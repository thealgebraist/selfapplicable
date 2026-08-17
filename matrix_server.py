#!/usr/bin/env python3
"""Tiny dependency-free live Markdown-to-HTML server for WORK_MATRIX.md."""

from html import escape
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import argparse


ROOT = Path(__file__).resolve().parent


def inline(text: str) -> str:
    result = escape(text)
    result = result.replace("`", "<code>", 1) if "`" in result else result
    if "</code>" not in result and "<code>" in result:
        result += "</code>"
    result = result.replace("**", "<strong>", 1) if "**" in result else result
    if "<strong>" in result and "</strong>" not in result:
        result += "</strong>"
    return result


def render_markdown(source: str) -> str:
    lines = source.splitlines()
    out = []
    i = 0
    in_code = False
    while i < len(lines):
        line = lines[i]
        if line.startswith("```"):
            if in_code:
                out.append("</code></pre>")
            else:
                out.append("<pre><code>")
            in_code = not in_code
        elif in_code:
            out.append(escape(line) + "\n")
        elif line.startswith("|") and i + 1 < len(lines) and lines[i + 1].startswith("|"):
            rows = []
            while i < len(lines) and lines[i].startswith("|"):
                cells = [c.strip() for c in lines[i].strip("|").split("|")]
                if not all(set(c) <= set("-: ") for c in cells):
                    rows.append(cells)
                i += 1
            if rows:
                out.append("<table><thead><tr>" + "".join(f"<th>{inline(c)}</th>" for c in rows[0]) + "</tr></thead><tbody>")
                for row in rows[1:]:
                    out.append("<tr>" + "".join(f"<td>{inline(c)}</td>" for c in row) + "</tr>")
                out.append("</tbody></table>")
            continue
        elif line.startswith("### "):
            out.append(f"<h3>{inline(line[4:])}</h3>")
        elif line.startswith("## "):
            out.append(f"<h2>{inline(line[3:])}</h2>")
        elif line.startswith("# "):
            out.append(f"<h1>{inline(line[2:])}</h1>")
        elif line.startswith("- "):
            out.append(f"<p class=\"item\">{inline(line[2:])}</p>")
        elif line.strip():
            out.append(f"<p>{inline(line)}</p>")
        i += 1
    return "\n".join(out)


class Handler(SimpleHTTPRequestHandler):
    def do_GET(self):  # noqa: N802
        if self.path in ("/", "/index.html", "/WORK_MATRIX.html"):
            body = render_markdown((ROOT / "WORK_MATRIX.md").read_text())
            page = """<!doctype html><meta charset=utf-8><title>Self-Applicable Work Matrix</title>
<style>body{font:16px system-ui;max-width:1400px;margin:2rem auto;padding:0 1rem;color:#18212b}table{border-collapse:collapse;width:100%}th,td{border:1px solid #ccd4dc;padding:.45rem;text-align:left;vertical-align:top}th{background:#edf2f7}code{background:#eef;padding:.1rem .25rem}.item{margin:.3rem 0}</style>
""" + body
            data = page.encode()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return
        if self.path == "/WORK_MATRIX.md":
            data = (ROOT / "WORK_MATRIX.md").read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/markdown; charset=utf-8")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return
        self.send_error(404)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()
    ThreadingHTTPServer(("127.0.0.1", args.port), Handler).serve_forever()
