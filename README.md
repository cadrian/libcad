[![Build Status](https://travis-ci.org/cadrian/libcad.png?branch=master)](https://travis-ci.org/cadrian/libcad)

# General description

Cad's Addenda... by Cyril Adrian (aka Cad)

`libCad` provides common services and functions for the utilities I
maintain (see e.g. [YacJP](https://github.com/cadrian/yacjp) and [Circus](https://github.com/cadrian/circus))

## Why yet another library

This library started with
[YacJP](https://github.com/cadrian/yacjp). Some parts are reusable
though, so I split them in a separate library.

Other parts I designed from scratch, based on ideas in other projects I work with.

## The libCad philosophy

C does not mean “not object”. Actually the whole Cad library is built
using object-oriented techniques: one may find encapsulation,
polymorphism, and even some design patterns (composites, factories,
visitors).

## Project building

`libCad` is a foundation library for other libraries to build upon. As
such, not only does it provide common code structures and functions,
but `libCad` also provides a build framework (*release.sh* and
*Makefile*) that should be able to build well-formed Debian packages
(the library package, a development package, and a documentation
package).
