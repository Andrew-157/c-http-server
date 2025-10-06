#!/usr/bin/python

import tomllib

with open("testcases/requestLine.toml", "rb") as f:
    data = tomllib.load(f)

print(data)
