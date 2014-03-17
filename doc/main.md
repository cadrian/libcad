# Introduction

## Why yet another library

This library started with YacJP. Some parts are reusable though, so I
split them in a separate library.

## The libCad philosophy

C does not mean ``not object''. Actually the whole Cad library is
built using object-oriented techniques: one may find encapsulation,
polymorphism, and even some design patterns (composites, factories,
visitors).

## Project building

libCad is a foundation library for other libraries to build upon. As
such, not only does it provide common code structures and functions,
but libCad also provides a build framework (`release.sh` and
`Makefile`) that should be able to build well-formed Debian packages
(the library package, a development package, and a documentation
package).

\mainpage libCad

libCad is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, version 3 of the License.

libCad is distributed in the hope that it will be useful, but __without
any warranty__; without even the implied warranty of
__merchantability__ or __fitness for a particular purpose__.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with libCad.  If not, see http://www.gnu.org/licenses/

Copyleft © 2012 Cyril ADRIAN


\defgroup cad_utils Tools

The most useful tool is used virtually everywhere: the memory
manager. The library uses a pair of functions similar to `malloc(3)`
and `free(3)` to handle memory chunks allocation and
deallocation. Those functions are available through a structure
`cad_memory_t` passed to most libCad public functions. The library
provides a standard ("stdlib") \ref stdlib_memory "memory manager" but
the user is free to provide her own.


\defgroup cad_hash Hash tables

The library provides a general-purpose hash table. It may be used
anywhere associative tables are needed.


\defgroup cad_event_queue Event queues

Event queues can be waited upon using event loops.


\defgroup cad_events Asynchronous event loops

Event loops are asynchronous event handlers.
