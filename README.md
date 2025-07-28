# Implementing HTTP Server in C

## Table of Contents
- [How To Read HTTP Request from Socket](#how-to-read-http-request-from-socket)
- [Inside a Virtual Machine](#inside-a-virtual-machine)
- [Client For Testing](#client-for-testing)
- [Future Features To Be Implemented](#future-features-to-be-implemented)

---

## How To Read HTTP Request from Socket

`recv` returns number of bytes read, which may be less than what the client sent, so we need to know for how long to call `recv`. We can rely on double `CRLF`(when no body) or `Content-Length` header when body is present. The big problem though is that if some malicious actor decides to send an invalid HTTP request, server may hang forever on waiting for double `CRLF` or the amount of data declared in `Content-Length`(assuming this header is present in the first place), so we should do the following:

- Set timeouts on reading from socket. I am not sure how it works, but if we are not getting full HTTP message after some timeout, it means the request is broken. Or instead of timeout we could, for example, call `recv` 5 times trying to read from socket. Either way, if we are not getting valid HTTP message, it means either the request is wrong or network is so unreliable that even TCP cannot deliver the application data, in both cases there is nothing server can do really, but to stop communication.

- Validate message as soon as you read it, so, for example, as soon as you see that request is invalid(no method field in request line, for example) - stop calling `recv`.

- Set a size limit to HTTP request: headers and body.

> `NOTE`: All of this will require thorough testing with some custom client.

---

## Inside a Virtual Machine

I have a Fedora 42 server libvirt VM on my host(I have virtual switch running in default `NAT` mode). To be able to run the HTTP server inside the VM and to be able to connect to the server from hypervisor(host), the port on which the server is running needs to be first opened on firewall(inside VM). Check first if port is already opened inside the VM with:

```bash
root@fedora-server:~# firewall-cmd --list-all
FedoraServer (default, active)
  target: default
  ingress-priority: 0
  egress-priority: 0
  icmp-block-inversion: no
  interfaces: enp1s0
  sources:
  services: cockpit dhcpv6-client ssh
  ports:
  protocols:
  forward: yes
  masquerade: no
  forward-ports:
  source-ports:
  icmp-blocks:
  rich rules:
```

Port which you want to use should be present in `ports` section. I want to use port **8080**, so to open it, run this:

```bash
root@fedora-server:~# firewall-cmd --add-port=8080/tcp
success
root@fedora-server:~# firewall-cmd --list-all
FedoraServer (default, active)
  target: default
  ingress-priority: 0
  egress-priority: 0
  icmp-block-inversion: no
  interfaces: enp1s0
  sources:
  services: cockpit dhcpv6-client ssh
  ports: 8080/tcp
  protocols:
  forward: yes
  masquerade: no
  forward-ports:
  source-ports:
  icmp-blocks:
  rich rules:
```

After this, it will be possible to connect to the HTTP server running inside the VM on the VM hypervisor using IP address of one of the interfaces of the VM.

> `WARNING`: This is not persistent and will not survive reboot, but for now it is enough for me.

> `IMPORTANT`: Setting `INADDR_ANY` as `hints.ai_flags` instead of `AI_PASSIVE` makes the server inaccessible from outside the virtual machine(it may be obvious for you, but not for me).

References:

- [https://www.digitalocean.com/community/tutorials/opening-a-port-on-linux](https://www.digitalocean.com/community/tutorials/opening-a-port-on-linux)

---

## Client For Testing

TODO

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

