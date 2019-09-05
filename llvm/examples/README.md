# DBrew Lifter for s2par Function Lifting

*Written for commit number [f40d26f7fb5d151112123c621501482245df782f](https://github.com/Hunterm267/dbrew/tree/f40d26f7fb5d151112123c621501482245df782f) of the lifter*

The DBrew instruction lifter currently works for s2par function lifting. In its current state, it will accept a single C function as its input, and produce LLVM-IR code. The C function needs to be provided as source code in the sample_funct.c file, its function signature added in the Lifter.h file, and a reference to the new function added in the Lifter.c file.

### Building DBrew and the Lifter
At this time, building DBrew and the lifter is only supported by using Meson. Full instructions can be found at [the meson website](https://mesonbuild.com/Quick-guide.html), but the necessary instructions are:
```
$ sudo apt-get install python3 python3-pip ninja-build
$ pip3 install --user meson
```
Afterward, navigate to the root DBrew directory, and create a new directory 'build':
```mkdir build```
Next, use the following commands to build the project:
```
meson build
cd build
ninja
```
At this point, the project should have built with no errors (and possibly a few non-critical warnings). Within your build directory, you can now navigate to `llvm/examples`. There should be a `lifter` executable here. Execute this, and the function you defined will be lifted.

### Changing the lifted function
To change the function that's being lifted, first navigate to the **source** directories. It should exist at:
`<DBREW ROOT>/llvm/examples`. 
1. First, edit the `sample_funct.c` file, and add your C source code.
2. Edit the `lifter.h` file, and add the function signature for the function added.
3. Edit the `lifter.c` file, and update `.name = "YOUR FUNCTION",` on line 26, and `(uintptr_t) YOUR FUNCTION` on line 32.
4. Return to your build directory - `<DBREW ROOT>/build`, and call `ninja` again. Then, refer to the instructions above to locate the `lifter` executable.