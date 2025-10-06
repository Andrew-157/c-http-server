# Testing Framework

## Table of Contents
- [Introduction](#introduction)

---

## Introduction

It is a pretty raw idea, but anyway better than nothing.

Testcases will be described using TOML files, for example:

```toml

title = "Testcases for testing RequestLine Parsing"

[Cases]

[Cases."Valid Request Line"]
description = "Test when Request Line is valid"
request = """\
GET /hello-c-http-server HTTP/1.1\r\n\r\n
"""
expected_response = """
HTTP/1.1 200 OK\r\n\
Content-Type: text/plain\r\n
"""
```

The cases will be run using a Python script that would use 2 binaries: server itself and client.

> `NOTE`: Why not use python requests? I don't know if Python requests allows for controlling the request to the extent I need.

> `NOTE`: Why not just use some existing framework, like robot? I don't know, for now, I don't want to spend time on setting up configuration for a testing framework, and I just want to keep writing the server itself. Moreover, writing your own framework, even if it barely works myst be pretty fun.
