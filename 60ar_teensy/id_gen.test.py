#!/usr/bin/env python3
"""
create a list of (1000) ids for testing

outputs a plain text file with JSON formatting
	due to arduino file limitations (8+3)

can be run via python(3) or  $ chmod +x id_gen.test.py && ./id_gen...
"""
import sys
import random

with open("ids.txt", "w", encoding="text/plain") as sys.stdout:
    ACC = []
    for i in range(1000):
        NUM = round(random.randint(1, 1000000000))
        NUM_STR = str(NUM)
        while len(NUM_STR) < 10:
            NUM_STR = "0" + NUM_STR
        ACC.append(NUM_STR)
    print({"url": "www.example.com", "ids": ACC})
