Iterators
=========

Iterators are one of several internal APIs used by funlisp but not exposed in
the public API. They are similar to the lazy evaluation constructs supported by
other languages (e.g. generators in Python), but for C. Here is what an iterator
looks like:

.. code:: C

  struct iterator {
      void *ds;        /* the container data structure */
      int index;       /* zero-based index for the iterator */
      int state_int;   /* some state variables that may help */
      void *state_ptr;

      /* do we have a next item? */
      bool (*has_next)(struct iterator *iter);

      /* return the next item (or null) */
      void *(*next)(struct iterator *iter);

      /* free resources held by the iterator */
      void (*close)(struct iterator *iter);
  };

The iterator holds some state, and three operations:

- has next: tell us whether the iterator can be advanced
- next: advance the iterator and return the next value
- close: free any resources held by the iterator

Any data structure could implement these (e.g. an array, a linked list, a hash
table, or others), and could be iterated over by code that knows nothing about
the underlying implementation. Iterators can also be composed in interesting
ways, with some of the helpers given below:

- empty iterator: return an empty iterator
- single value iterator: return an iterator that contains one item
- concatenate: given an array of iterators, yield from each of them until they
  run out
