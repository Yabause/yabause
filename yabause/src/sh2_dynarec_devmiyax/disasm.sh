#!/bin/bash

objdump -b binary -D -maarch64 $1 > $1.txt

