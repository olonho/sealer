ELF file symbols sealer.

Theory of operations.

When statically linking with libstdc++ we could encounter
rather nasty side effects coming from ODR and conflicts with system libstdc++.so. 
So to make library fully "sealed" we introduce a tool which modifies all
symbols but having certain prefix to be hidden by default and thus not interferring 
with system libstc++.

Actual selection of how to hide symbol may vary, and the least intrustive way seems
to set ELF sysmbol's `st_other` field to STV_PROTECTED (see 
https://sources.debian.org/src/glibc/2.19-18+deb8u9/elf/dl-lookup.c/#L788 for additional info).
If required, more aggressive mechanisms, such as complete symbol rename is possible.
