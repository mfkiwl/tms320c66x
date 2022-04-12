# !/usr/bin/env python
# -*- coding:utf-8 -*-  

from idaapi import *

cnt = 0x1000
adr = 0
fd = open("E:\\LYW\\iridium\\tt4300\\firmware\\BCX\\srcipt\\dsp\\compare\\ida_asm.txt","w")
while(cnt > 0):
	text = GetDisasm(adr)
	for i in range(0,6):
		text = text.replace("  ", " ")
	text = text.replace(" .", ".")
	text = text.replace(", ", ",")
	text = text.strip()
	fd.write(text+"\n")
	adr = next_not_tail(adr)
	cnt -= 1
fd.close()

fd = open("E:\\LYW\\iridium\\tt4300\\firmware\\BCX\\srcipt\\dsp\\compare\\asm0.txt", "r")
asm_text = fd.read()
fd.close()

def analyze(line):
	adr = int(line[0:8],16)
	data = int(line[11:19],16)
	text = line[19:]
	opcode = text.split(" ")
	while "" in opcode:
		opcode.remove("")
	return {adr:[adr,data,text,opcode,0]}

asmt = {}
for line in asm_text.split("\n"):
	asmt.update(analyze(line))
	
fd = open("E:\\LYW\\iridium\\tt4300\\firmware\\BCX\\srcipt\\dsp\\compare\\asm.txt", "w")
cnt = 0x1000
for key in sorted(asmt):
	if(cnt <= 0):
		break
	text = asmt[key][2]
	for i in range(0,6):
		text = text.replace("  ", " ")
	text = text.replace("| [","|[")
	text = text.replace("[ ","[")
	text = text.strip()
	fd.write(text+"\n")
	cnt -= 1
fd.close()
