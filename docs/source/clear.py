import os
import shutil

def clearDir(path):
    if os.path.exists(path):
        shutil.rmtree(path)
        print("Cleared " + path + "!")
        return
    print("Not existing path " + path)

if __name__ == "__main__":
    clearDir("/home/mdjur/program/vistle/docs/build")
