## libmongrel2

libmongrel2 is a library for writing [Mongrel2][m2] handlers.
It handles setting up communication with a Mongrel2 instance,
parsing the received requests and sending well-formatted replies.

### Requirements

The only external requirement is [ØMQ][zmq] (ZeroMQ) version 3.2+,
(or any version earlier than 3.0, with compatibility macros) which
is needed to communicate with the Mongrel2 instance.

### Building and Installation

There is a Makefile for building it. It has been tested using clang, and
seems to work. I am happy to accept changes to make it better/sane/cross-
platform, but I only have Linux machines to test on.

The build produces a shared library and there should (theoretically) be a
target for installing it.

### Usage

The library is intended to be simple to use.

There is one header to include, `mongrel2.h`, and one library to link against,
`libmongrel2.so` (so use `-lmongrel2`). The functions are documented in the
header file and should be fairly easy to use.

#### BStrings

This library makes heavy use of the `bstring` library. It directly includes the
source code, so there is no external dependency. This is something to watch out
for, especially if you are planning to use bstrings yourself.

#### API style

The library does its own allocation, and provides functions for freeing objects
to match the ones that create them. Almost always, the caller is responsible for
the arguments it passes to a function, any exceptions are documented on the
particular function.

#### Thread-safety

The library has a similar level of thread-safety to ØMQ. This means that contexts
can be passed around safely and connections cannot be used from multiple threads.

No other data structures are "thread-safe" in terms of access, though there are no
issues with passing objects between threads. The library holds no internal references
to any of the data structures it creates, meaning that there is no hidden concurrent
access (e.g. when you call `m2_request_get_header` on a request only the data inside
and referenced by that request are accessed).

#### TNetstrings

Mongrel2, by default, passes the headers in JSON format. This is to maintain backwards
compatability with older versions and older handlers. However, there is an option for
handlers to use TNetstrings as the format. Setting `protocol = "tnetstrings"` in the
handler configuration will do the switch.

TNetstrings are much easier to parse and are effectively the native format inside newer
versions of Mongrel2. libmongrel2 is much better at handling TNetstrings and it is
strongly recommended to enable them in your Mongrel2 configuration. libmongrel2 handles
both formats seamlessly however, so it is not necessary.

### Relationship to other handler libraries

There is one other handler library for C. It is linked to by the Mongrel2 website,
however that version is out of date. The more up-to-date fork is fine, however it doesn't
build against modern versions of ØMQ.

This library intends to be simpler, possibly at the expense of making certain patterns
less efficient. However, most people are going to be using a single connection and
each request will likely be handled in isolation.

### Future

Look at TODO.md to get an idea what the future plans are. As always, the only reliable
source of information about the state of the code is the code itself.

Until the library reaches some sort of "stable" position, the API is considered fair game
for changes.

[m2]:   http://www.mongrel2.org "Mongrel2"
[zmq]:  http://www.zeromq.org "ZeroMQ"
