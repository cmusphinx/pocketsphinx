#!/usr/bin/python

import sys

latfile = open(sys.argv[1], "r")

print """
digraph lattice {
	rankdir=LR;
"""

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

