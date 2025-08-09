# Corner Cases for the server to handle

## Table of Contents
- [Testing Framework](#testing-framework)
- [Conventions](#conventions)
- [Boundries Cases](#boundries-cases)
- [Format Cases](#format-cases)

---

## Testing Framework

Using the testing client program, different formats of messages can be sent, timeouts can be simulated, etc, and the response of the server can be validated and checked whether server responded as expected.

> `TODO`: Introduce some config file, where each testcase could be described using some common format, and this file could then be passed to the client program, which would run those testcases.

---

## Conventions

Server has the following behavior and features:

- Size of request line + Headers has a limit
- Size of the body has a limit
- `TODO`: handling of Content-Length specifying value bigger than the actual received body
- `TODO`: handling of Content-Length specifying value less than the actual body

---

## Boundries Cases

By "boundries" I mean: separation of request lines, headers, and body, presence of the double CRLF, and sizes of request line, headers and body.

Cases:

- Request consists only of request line, but request line doesn't end with double CRLF.
- Request consists only of request line, but request line ends with single CRLF.
- Request consists only of request line, but request line exceeds in size the maximum size of request line + headers.
- Request consists only of request line, and request line is valid.

---

## Format Cases

By "format" I mean compliance of the HTTP request message with the RFC.
