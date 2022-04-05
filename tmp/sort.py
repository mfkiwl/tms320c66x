fd = open("table","r")
lines = fd.read().split("\n")
fd.close()

list = {}
for line in lines:
	if(line == ''):
		break
	ele = line.split(" ")
	id = int(ele[4][2:],2)
	if(id in list):
		print(line)
		print(list[id])
		print()
		continue
	list[id] = line
	# print(line)

for l in list.sort():
	print(list[l])