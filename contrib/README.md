Contents of this directory
==========================

This directory collects files that sometimes can be helpful when using Vistle.
Use them as a starting point to add your own modifications.

- `direnv-envrc`:
  *symlink to `../.envrc` for use with [direnv](https://direnv.net)*
  This script tries to set up environment variables to build in and run from the
  subdirectory `$VMAKE_BUILD` of the source tree.

- `DebugWithXcode.xcodeproj`:
  *dummy project for Xcode in order to provide a workspace where attaching a debugger to running processes is possible*

- `vistle.natvis`:
  *Vistle types display style for MSVC debugger, add 
  "visualizerFile" : "${workspaceFolder}/contrib/vistle.natvis"
  to your launch.json configuration*

- `scripts`:
  *various scripts, e.g. for use with batch systems*
