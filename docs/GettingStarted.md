# Getting Started

To use *feel* in your project, just include the folder `libfeel/include`.

# Header only with cmake

You can add the `libfeel` directory into your project and include it in your `CmakeLists.txt` file:

```cmake
add_subdirectory(libfeel)
```

Ensure all git submodules have been initialized.

# Building the documentation

To create the documentation of *feel* you need to have Doxygen installed.

To build it run:
```
doxygen Doxyfile
```