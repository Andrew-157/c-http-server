# Implementing HTTP Server in C

## Table of Contents
- [How To Read HTTP Request from Socket](#how-to-read-http-request-from-socket)
- [Future Features To Be Implemented](#future-features-to-be-implemented)

---

## How To Read HTTP Request from Socket

`recv` returns number of bytes read, which may be less than what the client sent, so we need to know for how long to call `recv`. We can rely on double `CRLF`(when no body) or `Content-Length` header when body is present. The big problem though is that if some malicious actor decides to send an invalid HTTP request, server may hang forever on waiting for double `CRLF` or the amount of data declared in `Content-Length`(assuming this header is present in the first place), so we should do the following:

- Set timeouts on reading from socket. I am not sure how it works, but if we are not getting full HTTP message after some timeout, it means the request is broken. Or instead of timeout we could, for example, call `recv` 5 times trying to read from socket. Either way, if we are not getting valid HTTP message, it means either the request is wrong or network is so unreliable that even TCP cannot deliver the application data, in both cases there is nothing server can do really, but to stop communication.

- Validate message as soon as you read it, so, for example, as soon as you see that request is invalid(no method field in request line, for example) - stop calling `recv`.

- Set a size limit to HTTP request: headers and body.

> `NOTE`: All of this will require thorough testing with some custom client.

---

## Future Features To Be Implemented

- **Implement logger**  
  Log to `stdout` and to `/var/log/<somefile.log>`.
  
- **Implement server configuration**  
  Allow choosing server port via CLI, a configuration file, or both.
  
- **Add a client**  
  Simulate sending:
  - Chunked requests  
  - Requests of wrong format, etc.

