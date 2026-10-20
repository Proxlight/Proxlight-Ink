#!/usr/bin/env python3
"""Regenerates ereader/page.h from web/index.html.  Usage: python3 tools/make_page.py"""
import pathlib
root = pathlib.Path(__file__).resolve().parent.parent
html = (root / "web" / "index.html").read_text(encoding="utf-8")
assert ')HTML"' not in html
(root / "ereader" / "page.h").write_text(
    '// Proxlight Ink web page served by the e-reader (generated from web/index.html by tools/make_page.py)\n'
    '#pragma once\n#include <Arduino.h>\nconst char PAGE[] PROGMEM = R"HTML(' + html + ')HTML";\n', encoding="utf-8")
print("page.h written,", len(html), "bytes of HTML")
