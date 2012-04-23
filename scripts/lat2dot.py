#!/usr/bin/python

import sys

def lattice_s3(latfile):
    mode="empty"
    for line in latfile:

	if line.startswith("Nodes"):
	    mode = "node"
	    continue
	if line.startswith("Edges"):
	    mode = "edge"
	    continue
	if line.startswith("#") or line.startswith("End"):
	    mode = "none"
	    continue
	
        items = line.strip().split(" ")
	if mode == "node":
	    if items[1] != "":
		print items[0] + " [label = \"" + items[1] + " " + items[2] + " " + items[3] + " " + items[4] + "\"];"
	    else:
		print "node " + items[0] + ";"
        if mode == "edge":
    	    print items[0] + " -> " + items[1] + " [label = \"" + items[2] + "\"];"
    print "}"

def create_map(items):
    dct = {}
    for i in items:
	dct[i[0]] = i[2:]
    return dct

def lattice_htk_wordnode(latfile):
    mode="empty"
    for line in latfile:
        items = line.strip().split()
	if items[0].startswith("I="):
	    dct = create_map(items)
    	    if dct.has_key("W"):
    		print dct["J"] + " [label = \"" + dct[W] + "\"];"
        if items[0].startswith("J="):
	    dct = create_map(items)
    	    if dct.has_key("W"):
        	print dct["S"] + " -> " + dct["E"] + " [label = \"" + dct["W"] + "," + dct["a"] + "," + dct["l"] + "\"];"
    	    else:
        	print dct["S"] + " -> " + dct["E"] + " [label = \"" + dct["a"] + "," + dct["l"] + "\"];"
    print "}"

if __name__ == '__main__':
    latfilename = sys.argv[1]
    latfile = open(latfilename, "r")

    print """
    digraph lattice {
	rankdir=LR;
    """

    if latfilename.endswith("slf"):
	lattice_htk_wordnode(latfile)
    else:
	lattice_s3(latfile)
