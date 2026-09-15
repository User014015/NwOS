it can compile files .nw and launch in the qemu

to do this you need:

download my nwimg.exe (it's compiler, source code: tools/nwimg.c)

make file: (NwDev/Name.nw)

enter this:

```text

#include <nwc.h>
int main() {
    nw::out << "Hello!\n";
    return 0;
}

```

Compile it:

Download: nwc.exe (compiler into .nwo, source code: compiler/compilerMain.c)

run in vscode: ./PATH/TO/nwc.exe /PATH/TO/test.nw

then run this: tools/nwimg.exe add NwOS.img NwDev/test.nw test.nw

(make sure that: you has nwimg.exe at tools, or make your path)

(make sure that: you has NwOS.img, and right path)

(make sure that: you has: file.nw, or any other file with .nw at path: NwDev/)

if you compiled right: launch NwOS from qemu -> type: run:
``` text

run <your file name.nwo>

```

make sure that file format is .nwo to launch it

or you can edit .nw


## NwC TUTORIAL

Basic syntax:

```text

Comments right now not supported.

1A - int
flt - float
lng - long
char var[32] - char (like C)
str var - string

#include <nwc.h>

nw::out << "Hey!\n";
nw::out << "Hey! << nw::endl;

nw::cin >> var
nw::line(nw::cin, var) // type all line

```