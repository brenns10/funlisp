Type System
===========

As previous pages have mentions, lisp objects rely on a mark and sweep garbage
collection system to manage their lifetimes. This page describes how the type
system is implemented, which includes the garbage collection implementation.

In Java, everything is an object. In this language, everything is a
:c:type:`lisp_value`.  This means two things:

1. Every object contains a ``type`` pointer.
2. Every object has a variable to store its ``mark``.
3. Every object has a pointer to the object allocated after it, forming a linked
   list.

Every type declares these using the ``LISP_VALUE_HEAD`` macro, like so:

.. code:: C

   typedef struct {
     LISP_VALUE_HEAD;
     int x;
   } lisp_integer;

You can cast pointers to these objects to ``lisp_value*`` and still access the
type object. All objects are passed around as ``lisp_value*``.

In order to allow objects to be treated differently based on their type (but in
a generic way to calling code), we use the type object.

A type object is just another object, with type :c:type:`lisp_type`. However, it
is NOT managed by the garbage collector. It contains the string name of a type,
along with pointers to implementations for the following functions: print, new,
free, eval, call, and expand. Therefore, if you have ``lisp_value *object`` and
you want to print it, you can do:

.. code:: C

   object->type->print(stdout, object);

Unfortunately that's very verbose. To simplify, each of the functions
implemented by a type object has an associated helper function. So you can
instead do:

.. code:: C

   lisp_print(stdout, object);

But there's no magic or switch statements involved here--we're simply using the
type object.

This means that it's not too difficult to add a type to the language! All you
need to do is declare a struct for your type, implement the basic functions, and
create a type object for it. The type object must implement the following
operations:

- print: writes a representation of the object to a file, without newline
- new: allocates and initializes a new instance of the object
- free: cleans up and frees an instance
- expand: creates an iterator of ALL references to objects this object owns
- eval: evaluate this in a scope
- call: call this item in a scope with arguments
