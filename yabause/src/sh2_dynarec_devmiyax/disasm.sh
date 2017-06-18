#!/bin/bash

objdump -b binary -D -marm $1 > $1.txt

