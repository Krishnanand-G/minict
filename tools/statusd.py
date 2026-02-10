#!/usr/bin/env python3
"""Tiny localhost server for .minict/status.json"""
import argparse
import json
import os
from http.server import BaseHTTPRequestHandler, HTTPServer

parser = argparse.ArgumentParser()
parser.add_argument("--port", type=int, default=7474)
parser.add_argument("--state", default=os.environ.get("MINICT_STATE_DIR", ".minict"))
args = parser.parse_args()


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/status":
            self.send_error(404)
            return
        path = os.path.join(args.state, "status.json")
        try:
            with open(path, encoding="utf-8") as f:
                body = f.read()
        except OSError:
            body = json.dumps({"runtime": "minict", "containers": []})
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body.encode())

    def log_message(self, *_):
        pass


HTTPServer(("127.0.0.1", args.port), Handler).serve_forever()
