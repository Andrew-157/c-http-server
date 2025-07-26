# Trying to write a primitive HTTP server in C

TODO:

- implement message parser
- implement listening server
- implement logger to log to stdout and to /var/log/<somefile.log>
- implement way to configure server(e.g. to choose server port) either via cli or via a config file or both
- Add a client to be able to simulate sending chunked requests, requests of wrong format, etc.
- Trying writing response in 2 calls to write

Open Questions:

- How to read from socket, I mean for how long? Is just calling `recv` until it returns 0 enough?
