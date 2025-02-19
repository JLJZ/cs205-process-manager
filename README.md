# CS205 - Operating Systems Assignment 2

Description: A process managing command line interactive shell

## Source

```
├── Makefile
├── README.md
├── build.sh
└── src
    ├── argparse.c
    ├── argparse.h
    ├── main.c
    ├── procman.c
    ├── procman.h
    └── prog.c
```

### Inside ./src

- `main.c` - main entry point and user input
- `procman.c` - process manager
- `argparse.c` - command parsing

### Using `build.sh`

```sh
chmod +x ./build.sh
./build.sh
```

### Using `make`

```sh
make prog
make
```
