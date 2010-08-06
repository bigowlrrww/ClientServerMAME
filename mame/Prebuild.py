import os,sys

basePath = ''
allfiles = []
subfiles = []

for root, dirs, files in os.walk(basePath):
	for f in files:
		allfiles.append(os.path.join(root, f))
		if root != basePath: # I'm in a subdirectory
			subfiles.append(os.path.join(root, f))

for filename in allfiles:
	if os.path.splitext(filename)[1].upper()=='.LAY':
		command = os.path.join("src","build","file2str")
		newfile = os.path.splitext(filename)[0]+'.lh'
		os.system(command+" "+filename+" "+newfile+" layout_"+os.path.splitext(os.path.split(filename)[1])[0])
		print (command+" "+filename+" "+newfile+" layout_"+os.path.splitext(os.path.split(filename)[1])[0])

	if os.path.splitext(filename)[1].upper()=='.PNG':
		command1 = os.path.join("out","png2bdc")
		newfile1 = os.path.splitext(filename)[0]+'.bdc'
		os.system(command1+" "+filename+" "+newfile1)
		print (command1+" "+filename+" "+newfile1)

		command2 = os.path.join("out","file2str")
		newfile2 = os.path.splitext(filename)[0]+'.fh'
		os.system(command2+" "+newfile1+" "+newfile2+" font_"+os.path.splitext(os.path.split(filename)[1])[0] + " UINT8")
		print (command2+" "+newfile1+" "+newfile2+" font_"+os.path.splitext(os.path.split(filename)[1])[0] + " UINT8")
