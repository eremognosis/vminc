# Documentation

## Uses
    This project is a vm to asm translator for nand2teris. It takes in a vm file and outputs an asm file that can be run on the hack platform. It is written in C and is designed to be easy to use and understand. It is also designed to be easy to extend and modify for future projects.

Assuming the executable is named `vminc`, you can run it using the command line as follows:

```bash
    ./vminc input.vm output.asm
```

This takes the input.vm and translkates to output.asm


```bash
    ./vminc input_directory output.asm
```
This checks all the .vm files in the input_directory and translates them to output.asm

```bash
    ./vminc input_directory
```
This checks all the .vm files in the input_directory and translates them to input_directory.asm


```bash
    ./vminc input.vm
```
This takes the input.vm and translkates to input.asm


````
    ./vminc input /path/to/output.asm
```
this checks all the .vm files in the input directory and translates them to output.asm
