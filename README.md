

# uzleo.json module

provides a C++ module to parse json content.

## Motivation

- in big project, using `nlohmann::json` header at multiple locations 
  kill build times. So motivated to write a minimalistic parser myself.
- learn `vibes coding` to do a json parser.

## Build and Use

### Dependencies

- `fmt` formatting library

### cmake flow

```bash
$ cmake -B build -S . -DCMAKE_CXX_COMPILER="clang++" -DCMAKE_CXX_FLAGS="-stdlib=libc++" -G "Ninja" -Dfmt_DIR=<path-to-fmt-config>
$ cmake --build build

# run tests
$ build/test
```

### API Documentation

generate `doxygen` documentation in `build/doc/html/`

```bash
$ python build_apidoc.py
```
