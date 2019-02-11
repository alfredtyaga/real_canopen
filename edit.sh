#!/bin/bash

source deactivate
source activate py27

cd canfestival/objdictgen

pythonw objdictedit.py ../../real-objdict.od
