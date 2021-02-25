ELF file symbols sealer.

Theory of operations.

When statically linking with libstdc++ we could encounter
rather nasty side effects coming from ODR and conflicts with system libstdc++.* So to make library fully "sealed" we introduce a tool which modifies all
symbols but having certain prefix to be hidden by default and thus not 
interferring with system libstc++.
