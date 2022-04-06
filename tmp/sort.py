fd = open("table","r")
lines = fd.read().split("\n")
fd.close()

list = {}
multi = False
for line in lines:
	if(line == ''):
		break
	ele = line.split(" ")
	id = int(ele[4][2:],2)
	if(id in list):
		print(line)
		print(list[id])
		print("")
		multi = True
		continue
	list[id] = line
	# print(line)
	
if(multi == False):
	cnt = 0
	for l in sorted(list):
		while(cnt != l):
			print("null, none, none, none //%06s" % bin(cnt).replace("0b",""))
			cnt+=1
		print(list[l])
		cnt+=1