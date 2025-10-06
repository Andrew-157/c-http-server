# Implementing HTTP Server in C

## Table of Contents
- [Request Handling](#request-handling)
- [Inside a Virtual Machine](#inside-a-virtual-machine)
- [Caching](#caching)
- [Encoding](#encoding)
- [Persistent Connections And Pipelining](#persistent-connections-and-pipelining)
- [Proxy Mode](#proxy-mode)
- [Client For Testing](#client-for-testing)
- [Future Features To Be Implemented](#future-features-to-be-implemented)

---

## Request Handling

Request Handling is a very complex topic, see a separate document describing it [here](./docs/RequestHandling.md).

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

## Caching

`WORK IN PROGRESS`

---

## Encoding

- Content-Coding vs Transfer-Coding

As I understand, "Content-Coding" means that body itself is encoded, for example in `gzip` format. While "Transfer-Coding" means that that gzipped body may be chunked.

---

## Persistent Connections And Pipelining

`WORK IN PROGRESS`

- Chunked Transfer Coding

---

## Proxy Mode

`WORK IN PROGRESS`

---

## Client For Testing

> `TODO:` Consider using(or taking inspiration from): [posting](https://github.com/darrenburns/posting)

Client program that provides a way to test the server.

More see [here](./testing/testing_framework.md).

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

- **Add scripts for building container image with this HTTP server**
  - Containerfile
  - Dockerfile
  - buildah script
