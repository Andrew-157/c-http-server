# Request Handling

## Table of Contents

- [Miscellaneous](#miscellaneous)
  - [Persistent Connections](#persistent-connections)
  - [For how long to read request line and headers](#for-how-long-to-read-request-line-and-headers)
- [Request Line](#request-line)
  - [Request Method](#request-method)
  - [Request URI](#request-uri)
  - [HTTP Version](#http-version)

---

## Miscellaneous

### Persistent Connections

One of the first important things to be decided when handling a request is whether the connection should be persistent or not.

According to [HTTP/1.1 RFC](https://datatracker.ietf.org/doc/html/rfc2616#section-8.1.2.1):

```
   An HTTP/1.1 server MAY assume that a HTTP/1.1 client intends to
   maintain a persistent connection unless a Connection header including
   the connection-token "close" was sent in the request. If the server
   chooses to close the connection immediately after sending the
   response, it SHOULD send a Connection header including the
   connection-token close.
```

So, when parsing the request, the header "Connection: close" indicates the sender wishes to close the connection after the request/response exchange, if this header is not present - keep the connection open.

> `NOTE`: What will happen with the client socket on the server side if client closes the connection? It is for situation when client doesn't send "Connection: close", so that we assume the they want to keep the connection open, but they close the connection.

What if the header contains some token other than close? - then close the connection, I don't think 4xx should be sent, but if client sends some shady values for headers, it is better to close the connection with that client.

### For how long to read request line and headers

Okay, when reading body, we have Content-Length and Transfer-Coding, what if buffer size for recv is too small for receiving whole request line and headers, how many more times should I call recv? Should I just let it timeout and if errno is a certain value assume that we are done? I guess if it times out it means we didn't read double CRLF and didn't get Content-Length or Transfer-Coding, so the request is malformed and we can send 4xx and it is not that big of a deal for a client to wait for ~1 sec, since they are receiving error anyway.

---

## Request Line

Before starting to parse the relevant data of the request line, according to [HTTP/1.1 RFC](https://datatracker.ietf.org/doc/html/rfc2616#section-4.1):

```
   In the interest of robustness, servers SHOULD ignore any empty
   line(s) received where a Request-Line is expected. In other words, if
   the server is reading the protocol stream at the beginning of a
   message and receives a CRLF first, it should ignore the CRLF.
```

I don't think I will be able to implement unconditionally compliant server anyway(I doubt that I will be able to implement conditionally compliant one too, though), so to make my life easier for now, I will ignore that SHOULD documentation and assume that request line starts with the first letter of the request method, otherwise the request is invalid(though, parsing algorithm then should take that into account that a request line may start with an empty line).

BNF of the request line:

```bnf
Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
```

Between entities of the request line, there should be only 1 space.

---

### Request Method

The allowed methods are: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT. The list of methods allowed by a resource can be specified in an Allow header field.

I didn't see anything that forbids you from sending some non-defined-by-the-standard method, and it is up to server to decide what to do with it. What I am going to do is to let users set the list of allowed methods on a resource, so that when the allowed methods are GET, HEAD, and POST, it doesn't matter if the request method is standard defined PUT or some CUSTOM_METHOD, response would anyway be 405 or 501.

### Request URI

According to [HTTP/1.1 RFC](#https://datatracker.ietf.org/doc/html/rfc2616#section-3.2.1):

```
   The HTTP protocol does not place any a priori limit on the length of
   a URI. Servers MUST be able to handle the URI of any resource they
   serve, and SHOULD be able to handle URIs of unbounded length if they
   provide GET-based forms that could generate such URIs. A server
   SHOULD return 414 (Request-URI Too Long) status if a URI is longer
   than the server can handle
```

So, I will try to handle url of any length, but if it proves too hard, I will set some configurable max length for the url. Should it be configurable by the user that url can be limitless? Wouldn't it be a backdoor for an attack, like you send a gigabyte long request uri and consume all memory of the server?

### HTTP Version

I am supporting only HTTP/1.1 for now, I know HTTP/1.1 servers should be backwards compatible with HTTP/1.0, but I won't implement it, for now, at least.


