# BACnet Stack Verifification and Validation Testing

CMake and C99 are used for the verification and validation testing.

## Testing in the Github workflow pipeline with Makefile

The Makefile 'test' recipe is used for the CMake recipe used in the pipeline.

The 'retest' can be used to re-build the test.

Changing to the test/build folder and running 'make test' can show
more details about a failing build or failing test. Running the actual
test by running ./test/build/bacnet/.../test_xyz has more detailed
results of running the test.

## Testing with VS Code with CMake extension and C/C++ build tools

* Open the test/ folder as a workspace.
* The folder structure follows the src/bacnet folder structure.
* Configure the CMake extension to use the C/C++ build tools kit.
* Configure the CMake extension to use the Debug build type.
* Build each or 'all' project using the CMake extension.
* Each unit or verification test can be build individually.
* The LCOV Cmake recipe will create Line-of-Code Coverage that
  can be viewed in a browser starting at test/build/lcoverage/index.html
* The CTest extension can also be used to run the tests.

## Validation and Unit testing with ctest or ztest

* The ctest suite is used from cmake managing the tests. Add new test
  files following the src/bacnet folder structure into the CMakeLists.txt
  file.  Copy and existing test folder at the same folder depth to start.
* The ztest framework (from Zephyr) has been ported to use in the tests
  and includes the fff mocking macros. The zassert_x() macros include a
  formatted print as the last argument so that print style debug can be
  used to show values that fail which aids in writing a test.
* Tests can also be written without the zassert_x() macros by including
  assert.h and using the assert() macro.

## Use pre-commit tools for changes and additions

* This library uses 'pre-commit' tools to format the files.
* Run pre-commit after staging a file and before committing
  to avoid pipeline build failures.
* If a file fails pre-commit checking, the file will be re-formated
  correctly by the pre-commit tool. Add the fixed file to the staged
  files and run pre-commit again.
