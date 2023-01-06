#!/usr/bin/python3
import string
import random
from sys import argv

size = int(argv[1]) 
N = 16
colmn = rec_str = ""
mis = 0
marks = 0
colmn = "NAME,MIS,SCORE\n"
fh = open("data.csv", "w")
fh.write(colmn)
mis_list = random.sample(range(1,size+1), size)
for i in range(size):
    mis = mis_list[i] 
    name = ''.join(random.choices(string.ascii_uppercase, k = N))
    marks = round(random.uniform(0.00, 100.00), 2)
    rec_str = name + ',' + str(mis) + ',' + str(marks) + '\n'
    fh.write(rec_str)
fh.close()
