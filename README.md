funtup
======

Functional programming with tuples of functions in c++11.


The library contains two main concepts from functional programming
implemented in c++11: compositions and batteries, which are
implemented using templates (which means that there is little or no
overhead compared to tailoring the equivalent functions by hand).

A composition feeds the output from one function to another function,
and so on until there are no more functions, and a value is returned.

A battery applies each function in a tuple to a single list of
parameters, and returns a corresponding tuple containing the return
values.

