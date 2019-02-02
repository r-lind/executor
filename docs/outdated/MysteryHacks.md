Executor Mystery Hacks
=====================

This is a list of "mystery hacks" found in Executor.
A mystery hack is something that that looks like it is being done
for obscure compatibility reasons, but where I don't know why.

MANDELSLOP
----------

When the stack is initialized (`mman.cpp`, `ROMlib_InitZones`), the stack pointer is decremented by `16 + 32 * 1024` bytes. These roughly `32KB` remain forever unused.

jmpl_to_ResourceStub
--------------------

Executor implements the undocumented OS trap `ResourceStub` (`0xA0FC`).
Then, in `main.cpp`, the trap is patched to a small piece of 68K code
that only consists of a `JMP` instruction jumping to Executor's original
implementation of the trap.
