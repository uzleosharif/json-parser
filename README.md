

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
$ build/test/test
```

### API Documentation

generate `doxygen` documentation in `build/doc/html/`

```bash
$ python build_apidoc.py
```

### Use in your project
```cpp
import uzleo.json;
import std;

auto main() -> int {
  auto json{uzleo::json::Parse("/tmp/test.json")};
  std::println("{}", json.Dump());
}
```

### non-cmake approach


```
# build std.pcm, fmt.pcm/o and json.pcm/o
$ mkdir -p build/uzleo
$ clang++ -std=c++26 -stdlib=libc++ -O3 <your-path-to-libc++>/v1/std.cppm --precompile -o build/std.pcm
$ clang++ -std=c++26 -stdlib=libc++ -O3 deps/fmt/src/fmt.cppm -fmodule-output -c -o build/fmt.o
$ clang++ -std=c++26 -stdlib=libc++ -O3 src/json.cppm -fmodule-file=std=build/std.pcm -fmodule-file=fmt=build/fmt.pcm -fmodule-output -c -o build/uzleo/json.o

# create lib archive
$ ar r build/uzleo/libjson.a lib/fmt.o lib/uzleo/json.o

# install std.pcm, fmt.pcm, json.pcm, libjson.a
$ cp build/std.pcm build/fmt.pcm build/uzleo/json.pcm build/uzleo/libjson.a install/.

# consume at client
$ clang++ -std=c++26 -stdlib=libc++ -O3 test.cpp -fmodule-file=uzleo.json=install/uzleo/json.cppm -fmodule-file=fmt=install/fmt.pcm -fmodule-file=std=install/std.pcm -o test -ljson -L install/
```


## Usage Notes

- Note that the main object `Json` provided by this module is intended to be 
a move-only type. Hence, users might see some limitations on STL style API usage.
For example, the `unordered_map` provided as `uzleo::json::Json::json_object_t` 
can not be populated with entries like:

```cpp
uzleo::json::Json::json_object_t my_obj{
{"foo", json{123.0}},
{"bar", json{true}}
};
```
will not compile (but same syntax works for e.g. `std::unordered_map<int, bool>`)

Due to move-only nature of value type for `json_object_t`, following is 
recommended to fill it:

```cpp
uzleo::json::Json::json_object_t my_obj{};
my_obj.emplace("foo", uzleo::json::Json{123.0});
my_obj.emplace("bar", uzleo::json::Json{true});
```


