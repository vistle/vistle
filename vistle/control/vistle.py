import sys;
from time import sleep;

import _vistle;
from _vistle import *;

_loaded_file = None

# print to network clients
class _stdout:
   def write(self, stuff):
      _vistle._print_output(stuff)

class _stderr:
   def write(self, stuff):
      _vistle._print_error(stuff)

# input from network clients
class _stdin:
   def readline(self):
      return _vistle._readline()

sys.stdout = _stdout()
sys.stderr = _stderr()
sys.stdin = _stdin()

#def _raw_input(prompt):
#   return _vistle._raw_input(prompt)

#__builtins__.raw_input = _vistle._raw_input

## redefine help
#python_help = help
#def help():
#   current_module = sys.modules[__name__]
#   python_help(current_module)

def getNumRunning():
   return len(getRunning())

def showRunning():
   running = getRunning()
   print "id\tname"
   for id in running:
      name = getModuleName(id)
      print id, "\t", name

def showBusy():
   busy = getBusy()
   print "id\tname"
   for id in busy:
      name = getModuleName(id)
      print id, "\t", name

def showInputPorts(id):
   ports = getInputPorts(id)
   for p in ports:
      print p

def showOutputPorts(id):
   ports = getOutputPorts(id)
   for p in ports:
      print p

def showConnections(id, port):
   conns = getConnections(id, port)
   print "id\tportname"
   for c in conns:
      print c.first, "\t", c.second

def showAllConnections():
   mods = getRunning()
   for m in mods:
      ports = getOutputPorts(m)
      for p in ports:
         conns = getConnections(m, p)
         for c in conns:
            print m, p, " -> ", c.first, c.second

def getParameter(id, name):
   t = getParameterType(id, name)
   
   if t == "Int":
      return getIntParam(id, name)
   elif t == "Float":
      return getFloatParam(id, name)
   elif t == "Vector":
      return getVectorParam(id, name)
   elif t == "String":
      return getStringParam(id, name)
   elif t == "None":
      return None

   return None

def getSavableParam(id, name):
   t = getParameterType(id, name)
   if t == "String":
      return "'"+getStringParam(id, name)+"'"
   else:
      return getParameter(id, name)

def showParameter(id, name):
   print getParameter(id, name)

def showParameters(id):
   params = getParameters(id)
   print "name\ttype\tvalue"
   for p in params:
      print p, "\t", getParameterType(id, p), "\t", getParameter(id, p)

def showAllParameters():
   mods = getRunning()
   print "id\tmodule\tname\ttype\tvalue"
   for m in mods:
      name = getModuleName(m)
      params = getParameters(m)
      for p in params:
          print m, "\t", name, "\t", p, "\t", getParameterType(m, p), "\t", getParameter(m, p)

def save(filename = None):
   global _loaded_file
   if filename == None:
      filename = _loaded_file

   f = open(filename, 'w')
   mods = getRunning()
   for m in mods:
      f.write("id"+str(m)+" = spawn('"+getModuleName(m)+"')\n")
      params = getParameters(m)
      for p in params:
         f.write("set"+getParameterType(m,p)+"Param(id"+str(m)+", '"+p+"', "+str(getSavableParam(m,p))+")\n")
      f.write("\n")

   for m in mods:
      ports = getOutputPorts(m)
      for p in ports:
         conns = getConnections(m, p)
         for c in conns:
            f.write("connect("+str(m)+",'"+str(p)+"', "+str(c.first)+",'"+str(c.second)+"')\n")
   f.close()
   print("Data flow network saved to "+filename)

def reset():
   global _loaded_file
   mods = getRunning()
   for m in mods:
      kill(m)
   barrier()
   _vistle._resetModuleCounter()
   _loaded_file = None

def load(filename = None):
   global _loaded_file
   if filename == None:
      filename = _loaded_file
   if filename == None:
      print "File name required"
      return

   reset()

   source(filename)

   _loaded_file = filename
