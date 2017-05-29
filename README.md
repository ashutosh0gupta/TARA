TARA
====

TARA is the implementation that accompanies our paper [1].

The tool accepts concurrent traces in a custom language or C++ programs and
returns a succinct error explanation in the form of a DNF over happens-before
relations under several memory models. This can be used for synthesis or
fault localisation.

[1] Ashutosh Gupta, Thomas A. Henzinger, Arjun Radhakrishna, Roopsha Samanta,
Thorsten Tarrach. Succinct Representation of Concurrent Trace Sets. In POPL
2015

Building on *nix
================

On Ubuntu 14.04 LTS
-------------------

(Tested on the x64 version. Should also run on the 32bit version)

Install the following dependencies:

	sudo apt-get -y install git g++ cmake libboost-regex-dev libboost-program-options-dev libboost-filesystem-dev libboost-system-dev flex bison

In a minimal debian install also the following tools are needed

	sudo apt-get -y install unzip python

- install llvm-3.8-dev and clang-3.8-dev (package name may not be exact)

Other *nix platforms
--------------------

Make sure you have installed on your machine

- git
- a recent version of GCC (version 4.7 or higher)
- flex (version 2.5 or higher)
- bison (version 2.4 or higher)
- boost
- llvm 3.8 (debug version on 4.0.0; installed during compilation)

Compiling
---------

To compile the debug version: navigate to the implementation folder and run

	cd implementation
	make debug

which will result in the file `tara` in the implementation folder.
For the release version type 

	make release

which will result in the file `tara` in the implementation folder.

To use the clang compiler issue `make clean` and then these commands before using make:

	export CC=/usr/bin/clang
	export CXX=/usr/bin/clang++

Building on Windows (32 bit) [outdated]
============================

Install the following dependencies:

- [GIT](http://git-scm.com/download/win) (during install choose "Use GIT from the Windows command prompt"
- [CMake 2.8](http://www.cmake.org/download/#previous)
- Boost, [procompiled for windows 32bit](http://sourceforge.net/projects/boost/files/boost-binaries/) (e.g. download boost_1_56_0-msvc-12.0-32.exe or newer) 
- [Win flex-bison](http://sourceforge.net/projects/winflexbison/) (extract to `c:\win_flex_bison`)
- [Python 2.7](https://www.python.org/download/releases/2.7/)
- Visual Studio 2013 (or [MS Windows SDK](http://msdn.microsoft.com/en-us/windows/desktop/bg162891.aspx)) with C++ compiler

Edit the system environment variables and add these to your PATH:
`C:\Program Files (x86)\CMake 2.8\bin`, `C:\Python27`, `c:\win_flex_bison`

Navigate to the C:\local\boost_1_56_0 directory and rename the `lib_<something>` folder to `lib`.
Add a new environment variable called `BOOST_ROOT` and set it to `C:\local\boost_1_56_0`

Compiling
---------

In the start menu open Visual Studio 2013 -> Visual Studio Tools. Then open "VS2013 x86 Native Tools Command Prompt" (A similar item is added by the SDK to the start menu).

Navigate to the implementation subdirectory of this release.

Then type `nmake /f Makefile.win`.

This will download Z3, patch it and build the executable `tara.exe`. A list of warnings will scroll by, these can be ignored.

To run `tara.exe` you will need to copy the following DLL to the directory where `tara.exe` is located:

- `libz3.dll` from `z3\buildr`
- `boost_filesystem-vc120-mt-1_56.dll` from `C:\local\boost_1_56_0\lib`
- `boost_program_options-vc120-mt-1_56.dll` from `C:\local\boost_1_56_0\lib`
- `boost_regex-vc120-mt-1_56.dll` from `C:\local\boost_1_56_0\lib`
- `boost_system-vc120-mt-1_56.dll` from `C:\local\boost_1_56_0\lib`
    
Command line options
====================

As a mandatory argument the file is needed.

	./tara [file]

Optional arguments consist of

| Option     | Values                       | Description                                             |
|------------|------------------------------|---------------------------------------------------------|
| `--ofile`  |                              | Add the output as a comment to the input file           |
| `--print`  | pruning,rounds,output,phi    | Print details about processing of this step             |
| `--mode`   | sepertate,synthesis,bugs     | See below                                               |

To enable full verbose printing add `--print=pruning,rounds,output,phi` to the command line.

Modes
-----

We support three modes: Trace sepertation into good and bad traces, synthesis of synchronisation and bug summarisation. Modes support additional options after a colon.

### Trace seperation

By default this mode prints out a DNF of the bad traces. `--mode=seperate`

Options include `bad_dnf`, `bad_cnf`, `good_dnf`, `good_cnf`, `verify`. Note that bad CNF and good DNF take longer to compute. `verify` means that in a seperate the result is verified to be correct. Verification can fail if there is non-determinism in the program without meaning the result is wrong.

Example: `--mode=seperate:good_cnf,verify`

### Synthesis of synchronisation

Invoke with `--mode=synthesis`. 

Options include `verify` and `nfs`. `nfs` prints the normal forms used by the synthesis.

### Bug summarisation

Invoke with `--mode=bugs`. 

Options include `verify` and `nfs`. `nfs` prints the normal forms used by the bug summarisation.

Input file format
=================

(this is not yet finalised)

	global: <var> ...
	
	⟨pre: (smt-formula)⟩
	
	thread <name> ⟨{<var>}⟩:
	⟨<name> :⟩ ⟨havok ( <name> ... )⟩ (smt-formula)
	⟨<name> :⟩ assume(smt-formula)
	⟨<name> :⟩ assert(smt-formula)
	... (more instructions)
	
	... (more threads)
	
	⟨atomic: [<name> <name>] (...)⟩
	⟨order: <name> < <name> (...)⟩


Parts in ⟨⟩ are optional. 

Global is used to declare global variables in the format `<name> | <name>:<type>`. `<name>` is alphanumeric and needs to start with a letter. `<type>` can be `int`, `bool`, `real`, `bv16`, `bv32`, `bv64` or any smt type in parenthesis.

`pre` is followed by a formula that gives the preconditions for the global variables.

Each `thread` has a `<name>` and an optional declaration of local variables in curly braces. After the thread follow the instructions that can have an optional name followed by a colon. The instruction is an SMT formula that may be preceeded by an assume or assert.

`havok` is followed by a list of variables that will be assigned an arbitrary value. These variables need not be mentioned in the smt-formula, but if they are they must be primed. It is possible to restrict the value of a havoked variable in the smt-formula. The reason why havoked variables should be listed explicitly is because they are treated as input to the program.

An instruction may refer to primed variables by adding a dot to the variable (`x.` for variable `x`).

Semantics of assume and asserts
-------------------------------

For a good trace all assumes and all asserts need to evaluate to true for all initial states and all non-deterministic choices.

For a bad trace there exists one initial state or non-deterministic choice so that one of the assertions fails, but all assumptions on the path to the assertion hold.

All other executions are considered infeasable.




Known issues
------------

- unsat_core pruning needs guards
- can thin be removed without worry
- C parsing: bool types are declared as 8bit 
- assert(false) support
- bulk rename litmus files.. and write their C versions
- todo: is_po_new add memoization
- delare the_launcher and the_finisher as sc
- populate all sc

LLVM 3.8 Bug
-------------

https://github.com/iovisor/bcc/issues/492

LLVM FIX as suggested in the above webpage

sudo apt-get install -y llvm-3.8-dev libclang-3.8-dev
sudo mkdir -p /usr/lib/llvm-3.8/share/llvm
sudo ln -s /usr/share/llvm-3.8/cmake /usr/lib/llvm-3.8/share/llvm/cmake
sudo sed -i -e '/get_filename_component(LLVM_INSTALL_PREFIX/ {s|^|#|}' -e '/^# Compute the installation prefix/i set(LLVM_INSTALL_PREFIX "/usr/lib/llvm-3.8")' /usr/lib/llvm-3.8/share/llvm/cmake/LLVMConfig.cmake
sudo sed -i '/_IMPORT_CHECK_TARGETS Polly/ {s|^|#|}' /usr/lib/llvm-3.8/share/llvm/cmake/LLVMExports-relwithdebinfo.cmake
sudo sed -i '/_IMPORT_CHECK_TARGETS sancov/ {s|^|#|}' /usr/lib/llvm-3.8/share/llvm/cmake/LLVMExports-relwithdebinfo.cmake
sudo ln -s /usr/lib/x86_64-linux-gnu/libLLVM-3.8.so.1 /usr/lib/llvm-3.8/lib/
