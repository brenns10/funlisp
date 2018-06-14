Garbage Collection
==================

Garbage collection is implemented in Lisp using a mark-and-sweep system. In this
system, values are first marked as to whether they are "reachable", and then all
objects which are unreachable can be freed.

To implement this, we need two key components. First, we need a way to mark
reachable objects. Second, we need a way to track all objects so that we can
find the unreachable ones to free. It turns out the second one is pretty easy:
just maintain a linked list of values as they are created. This logic is nicely
handled inside of ``lisp_new()``.

The second one is trickier. My strategy was for every type to implement its own
``expand()`` operation. This is a function which returns an iterator that yields
every reference the object contains. For example, strings and integers yield
empty iterators. However, lists yield their left and right objects (if they
exist). Scopes yield every symbol and then every value stored within them. They
also yield a reference to their parent scope, if it exists.

The mark operation then simply performs a breadth-first search. It starts with a
single value, marks it "black", then uses the expand operation. It goes through
each value, adding all "white" items to the queue, marking them "grey" as it
goes. Then it chooses the next item in the queue and does the same operation,
until the queue is empty.

To do the breadth-first search, we use a "ring buffer" implementation, which
implements a circular, dynamically expanding double-ended queue. It is quite
simple and useful. It can be found in ``src/ringbuf.c``.
