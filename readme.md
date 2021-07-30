# [Mat’s PreProcessor](https://github.com/matgat/mpp.git)

Simple macro expansion tool written with maximum speed in mind
* Lightweight
* Enhanced by *Poor man's Unicode* library, to (almost) support Unicode files


## Build
```
$ git clone https://github.com/matgat/mpp.git
```


## Usage
To preprocess `test.c` with `#defines` contained in `defvar.h`:
```
$ mpp -i defvar.h test.c
```
