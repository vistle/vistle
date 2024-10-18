Every `.toml` file in this directory and its subdirectories are picked up by
`lib/config/CMakeLists.txt` and baked into the binaries.
Use them to provide required defaults.

*The files here are not read during run time, but during build time!*
