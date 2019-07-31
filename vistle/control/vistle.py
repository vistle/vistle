import sys
from time import sleep

import _vistle
from _vistle import *

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

   if remote:
      f.write("# spawn all remote modules\n")
   else:
      f.write("# spawn all local modules\n")
   for m in mods:
      hub = _vistle.getHub(m)
      if (remote and hub==getMasterHub()) or (not remote and hub!=getMasterHub()):
            continue
      #f.write(modvar(m)+" = spawn('"+_vistle.getModuleName(m)+"')\n")
      f.write("u"+modvar(m)+" = spawnAsync("+hubVarForModule(m, numSlaves)+", '"+_vistle.getModuleName(m)+"')\n")
   f.write("\n")

   for m in mods:
      hub = _vistle.getHub(m)
      if (remote and hub==getMasterHub()) or (not remote and hub!=getMasterHub()):
            continue
      f.write(modvar(m)+" = waitForSpawn(u"+modvar(m)+")\n")
      saveParameters(f, m)

   if remote:
      f.write("# connections between local and remote\n")
   else:
      f.write("# all local connections\n")
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
   if filename == None:
      filename = getLoadedFile()
   if filename == None or filename == "":
      print("No file loaded and no file specified")
      return

   setLoadedFile(filename)

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
   mods = _vistle.getRunning()
   for m in mods:
      kill(m)
   barrier()
   #_vistle._resetModuleCounter()
   setLoadedFile("")
   setStatus("Workflow cleared")

def load(filename = None):
   if filename == None:
      filename = getLoadedFile()
   if filename == None or filename == "":
      print("File name required")
      return

   reset()

   source(filename)

   setLoadedFile(filename)
   setStatus("Workflow loaded: "+filename)


# re-export functions from _vistle
source = _vistle.source
spawn = _vistle.spawn
spawnAsync = _vistle.spawnAsync
waitForSpawn = _vistle.waitForSpawn
kill = _vistle.kill
connect = _vistle.connect
disconnect = _vistle.disconnect
compute = _vistle.compute
interrupt = _vistle.interrupt
quit = _vistle.quit
trace = _vistle.trace
barrier = _vistle.barrier
requestTunnel = _vistle.requestTunnel
removeTunnel = _vistle.removeTunnel
printInfo = _vistle.printInfo
printWarning = _vistle.printWarning
printError = _vistle.printError
setStatus = _vistle.setStatus
clearStatus = _vistle.clearStatus
setLoadedFile = _vistle.setLoadedFile
getLoadedFile = _vistle.getLoadedFile
setParam = _vistle.setParam
setIntParam = _vistle.setIntParam
setFloatParam = _vistle.setFloatParam
setStringParam = _vistle.setStringParam
setVectorParam = _vistle.setVectorParam
setIntVectorParam = _vistle.setIntVectorParam
applyParameters = _vistle.applyParameters
getAvailable = _vistle.getAvailable
getRunning = _vistle.getRunning
getBusy = _vistle.getBusy
getModuleName = _vistle.getModuleName
getInputPorts = _vistle.getInputPorts
waitForHub = _vistle.waitForHub
waitForHubs = _vistle.waitForHubs
waitForNamedHubs = _vistle.waitForNamedHubs
getMasterHub = _vistle.getMasterHub
getVistleSession = _vistle.getVistleSession
getAllHubs = _vistle.getAllHubs
getHub = _vistle.getHub
getOutputPorts = _vistle.getOutputPorts
getConnections = _vistle.getConnections
getParameters = _vistle.getParameters
getParameterType = _vistle.getParameterType
isParameterDefault = _vistle.isParameterDefault
getIntParam = _vistle.getIntParam
getFloatParam = _vistle.getFloatParam
getVectorParam = _vistle.getVectorParam
getIntVectorParam = _vistle.getIntVectorParam
getStringParam = _vistle.getStringParam
