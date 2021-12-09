import time
import os

def cacheUntil(file, tag):
    cache = ""
    print("caching until " + tag)
    while True:
        line = file.readline()
        pos = line.find(tag)
        if not line:
            break
        if pos == -1:
            cache += line
        else:
            cache += line[0 : pos]
            break
    return cache

def readUntil(file, tag):
    while True:
        line = file.readline()
        pos = line.find(tag)
        if not line:
            break
        if pos != -1:
            print("decreasing current pos " + str(file.tell()) + " by " + str(pos))
            if file.tell() - pos < 0:
                file.seek(0,0)
            else:
                file.seek(file.tell() - pos, 0)
            break

            

def cacheOldFile(file, comments): #returns a list witn len(comments) + 1 with content before and after the commented sections
    contents = []
    file.seek(0)
    for comment in comments:
        contents.append(cacheUntil(file, comment))
        readUntil(file, comment)
    return contents


file = open("/home/dennis/src/vistle/module/general/Sensei/README.md", "r")

comments = ["[headline]:<>", "[inputPorts]:<>", "[outputPorts]:<>", "[parameters]:<> ", "alsdjasklfhjgklasol"]
contents = cacheOldFile(file, comments)
for content in contents:
    print(content)
    print("----------------------------------------------------------------------------------")
