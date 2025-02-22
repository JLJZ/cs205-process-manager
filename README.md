# CS205 - Operating Systems Assignment 2

Description: A CLI process manager

```text
├── Makefile
├── build.sh
├── README.md
└── src
    ├── argparse.c
    ├── argparse.h
    ├── main.c
    ├── procman.c
    ├── procman.c
    ├── input.h
    ├── input.c
    └── prog.c
```

## Inside ./src

- `main.c` - main entry point and user input
- `procman.c` - process manager
- `argparse.c` - command parsing
- `input.c` - file descriptor reading utilities

## Using `build.sh`

```sh
chmod +x ./build.sh
./build.sh
```

## Using `make`

```sh
make prog
make
```
