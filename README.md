# pseudo-writer-first-lock
It is a pseudo writer first lock, as readers that come after the first writer would also be bloked if any later writer comes.

In other words, readers come after any writer should wait until there is no writers. 
