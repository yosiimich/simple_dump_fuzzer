# simple_dump_fuzzer
just simple dump fuzzer

## how to build
```
./build.sh
```

## how to execute
```
./main ./target ./testfile
```

## description
main.c -> fuzzer
forkserver.c -> forkserver injected into the target
compiler.c -> A compiler that embeds a forkserver into the target