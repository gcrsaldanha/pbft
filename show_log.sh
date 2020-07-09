#!/bin/bash
grep -Fw "[$1]" log.txt | echo
