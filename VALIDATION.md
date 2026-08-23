# Validation status

Validated in the current build environment:

- All `src/*.cpp` / `src/*.h` files are present in `CMakeLists.txt`.
- `config.example.json` parses successfully.
- `.github/workflows/windows.yml` parses successfully as YAML.
- Raw C++ brace balance check passes.
- `git diff --cached --check` passes (no whitespace errors).
- CMake configures far enough to identify the C++ compiler and reaches `find_package(Qt6 6.5 ...)`.

Not claimed as validated here:

- A real Windows/MSVC/Qt link/run was not possible in this Linux container because Qt6 development files are not installed. The included `windows.yml` workflow is the intended authoritative Windows build check once the project is pushed to a GitHub repository.
- Runtime interoperability with a particular installed Everything version, third-party file dialogs, and Shell extensions still needs Windows integration testing.
