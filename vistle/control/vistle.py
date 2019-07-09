import sys
from time import sleep

import _vistle
from _vistle import *

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

#sys.stdout = _stdout()
#sys.stderr = _stderr()
#sys.stdin = _stdin()

#def _raw_input(prompt):
#   return _vistle._raw_input(prompt)

#__builtins__.raw_input = _vistle._raw_input

## redefine help
#python_help = help
#def help():
#   current_module = sys.modules[__name__]
#   python_help(current_module)

def showHubs():
   hubs = _vistle.getAllHubs()
   for id in hubs:
      print(id)

def showAvailable():
   avail = _vistle.getAvailable()
   for name in avail:
      print(name)

def getNumRunning():
   return len(_vistle.getRunning())

def showRunning():
   running = _vistle.getRunning()
   print("id\tname\thub")
   for id in running:
      name = _vistle.getModuleName(id)
      hub = _vistle.getHub(id)
      print("%s\t%s\t%s" % (id, name, hub))

def showBusy():
   busy = getBusy()
   print("id\tname")
   for id in busy:
      name = _vistle.getModuleName(id)
      print("%s\t%s" % (id, name))

def showInputPorts(id):
   ports = _vistle.getInputPorts(id)
   for p in ports:
      print(p)

def showOutputPorts(id):
   ports = _vistle.getOutputPorts(id)
   for p in ports:
      print(p)

def showConnections(id, port):
   conns = _vistle.getConnections(id, port)
   print("id\tportname")
   for c in conns:
      print("%s\t%s" % (c[0], c[1]))

def showAllConnections():
   mods = _vistle.getRunning()
   for m in mods:
      ports = _vistle.getOutputPorts(m)
      ports.extend(getParameters(m))
      for p in ports:
         conns = _vistle.getConnections(m, p)
         for c in conns:
            print("%s:%s -> %s:%s" % (m, p, c[0], c[1]))

def getParameter(id, name):
   t = _vistle.getParameterType(id, name)

   if t == "Int":
      return _vistle.getIntParam(id, name)
   elif t == "Float":
      return _vistle.getFloatParam(id, name)
   elif t == "Vector":
      return _vistle.getVectorParam(id, name)
   elif t == "IntVector":
      return _vistle.getIntVectorParam(id, name)
   elif t == "String":
      return _vistle.getStringParam(id, name)
   elif t == "None":
      return None

   return None

def getSavableParam(id, name):
   t = getParameterType(id, name)
   if t == "String":
      return "'"+getStringParam(id, name)+"'"
   elif t == "Vector":
      v = getVectorParam(id, name)
      s = ''
      first=True
      for c in v:
         if not first:
            s += ', '
         first = False
         s += str(c)
      return s
   elif t == "IntVector":
      v = getIntVectorParam(id, name)
      s = ''
      first=True
      for c in v:
         if not first:
            s += ', '
         first = False
         s += str(c)
      return s
   else:
      return getParameter(id, name)

def showParameter(id, name):
   print(getParameter(id, name))

def showParameters(id):
   params = getParameters(id)
   print("name\ttype\tvalue")
   for p in params:
      print("%s\t%s\t%s" % (p, getParameterType(id, p), getParameter(id, p)))

def showAllParameters():
   mods = _vistle.getRunning()
   print("id\tmodule\tname\ttype\tvalue")
   for m in mods:
      name = _vistle.getModuleName(m)
      params = getParameters(m)
      for p in params:
          print("%s\t%s\t%s\t%s\t%s" % (m, name, p, getParameterType(m, p), getSavableParam(m, p)))

def modvar(id):
   if (id >= 0):
      return "m" + _vistle.getModuleName(id) + str(id)
   if (id == _vistle.getVistleSession()):
       return "VistleSession"
   return "v"+str(-id)

def hubVar(hub, numSlaves):
   if (hub == getMasterHub()):
      return "MasterHub"
   if (numSlaves == 1):
      return "SlaveHub"
   return "Slave"+str(-hub);

def hubVarForModule(id, numSlaves):
   hub = _vistle.getHub(id)
   return hubVar(hub, numSlaves)

def saveParameters(f, mod):
      params = getParameters(mod)
      paramChanges = False
      for p in params:
         if not isParameterDefault(mod, p):
            f.write("set"+getParameterType(mod,p)+"Param("+modvar(mod)+", '"+p+"', "+str(getSavableParam(mod,p))+", True)\n")
            paramChanges = True
      if paramChanges:
         f.write("applyParameters("+modvar(mod)+")\n")
      f.write("\n")

def saveWorkflow(f, mods, numSlaves, remote):
   f.write("\n")

   for m in mods:
      hub = _vistle.getHub(m)
      if (remote and hub==getMasterHub()) or (not remote and hub!=getMasterHub()):
            continue
      #f.write(modvar(m)+" = spawn('"+_vistle.getModuleName(m)+"')\n")
      f.write("u"+modvar(m)+" = spawnAsync("+hubVarForModule(m, numSlaves)+", '"+_vistle.getModuleName(m)+"')\n")

   for m in mods:
      hub = _vistle.getHub(m)
      if (remote and hub==getMasterHub()) or (not remote and hub!=getMasterHub()):
            continue
      f.write(modvar(m)+" = waitForSpawn(u"+modvar(m)+")\n")
      saveParameters(f, m)

   for m in mods:
      hub = _vistle.getHub(m)
      if (not remote and hub!=getMasterHub()):
            continue
      ports = getOutputPorts(m)
      for p in ports:
         conns = getConnections(m, p)
         for c in conns:
            hub2 = _vistle.getHub(c[0])
            if (remote and hub==getMasterHub() and hub2==getMasterHub()) or (not remote and (hub!=getMasterHub() or hub2!=getMasterHub())):
               continue
            f.write("connect("+modvar(m)+",'"+str(p)+"', "+modvar(c[0])+",'"+str(c[1])+"')\n")


def save(filename = None):
   global _loaded_file
   if filename == None:
      filename = _loaded_file
   if filename == None:
      print("No file loaded and no file specified")
      return

   f = open(filename, 'w')
   mods = _vistle.getRunning()

   master = getMasterHub()
   f.write("MasterHub=getMasterHub()\n")
   f.write("VistleSession=getVistleSession()\n")

   slavehubs = set()
   for m in mods:
      h = _vistle.getHub(m)
      if h != master:
         slavehubs.add(h)
   numSlaves = len(slavehubs)

   f.write("uuids = {}\n");

   saveParameters(f, getVistleSession())
   saveWorkflow(f, mods, numSlaves, False)

   if numSlaves > 1:
      print("slave hubs: %s" % slavehubs)
      f.write("slavehubs = waitForHubs("+str(numSlaves)+")\n")
      count = 0
      for h in slavehubs:
          f.write(hubVar(h, numSlaves) + " = slavehubs["+str(count)+"]\n")
          count = count+1
   elif numSlaves > 0:
      print("slave hubs: %s" % slavehubs)
      f.write("print('waiting for a slave hub to connect...')\n")
      f.write("printInfo('waiting for a slave hub to connect...')\n")
      f.write("SlaveHub=waitForHub()\n")
      f.write("print('slave hub %s connected\\n' % SlaveHub)\n")
      f.write("printInfo('slave hub %s connected\\n' % SlaveHub)\n")

   saveWorkflow(f, mods, numSlaves, True)

   #f.write("checkMessageQueue()\n")

   f.close()
   print("Data flow network saved to "+filename)

def reset():
   global _loaded_file
   mods = _vistle.getRunning()
   for m in mods:
      kill(m)
   barrier()
   #_vistle._resetModuleCounter()
   _loaded_file = None

def load(filename = None):
   global _loaded_file
   if filename == None:
      filename = _loaded_file
   if filename == None:
      print("File name required")
      return

   reset()

   source(filename)

   _loaded_file = filename
