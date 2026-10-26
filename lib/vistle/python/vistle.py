import _vistle
import re
import os

coGRMsgLoaded = False

def findFirstModule(moduleName):
   return _vistle.findFirstModule(moduleName)

def sendCoverMessage(msg, coverModuleId = -1):
   if coverModuleId == -1:
      coverModuleId = findFirstModule("COVER")
   _vistle.sendCoverMessage(msg.c_str(), coverModuleId)

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

def showAvailable(hub = _vistle.Id.MasterHub):
   avail = _vistle.getAvailable(hub)
   for name in avail:
      cat = _vistle.getModuleCategory(name, hub)
      desc = _vistle.getModuleDescription(name, hub)
      print(f"{name} ({cat}): {desc}")

def getNumRunning():
   return len(_vistle.getRunning())

def showRunning():
   running = _vistle.getRunning()
   print("id\tname\thub")
   for id in running:
      name = _vistle.getModuleName(id)
      hub = _vistle.getHub(id)
      print(f"{id}\t{name}\t{hub}")

def showBusy():
   busy = getBusy()
   print("id\tname")
   for id in busy:
      name = _vistle.getModuleName(id)
      print(f"{id}\t{name}")

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
      print(f"{c[0]}\t{c[1]}")

def showAllConnections():
   mods = _vistle.getRunning()
   for m in mods:
      ports = _vistle.getOutputPorts(m)
      ports.extend(getParameters(m))
      for p in ports:
         conns = _vistle.getConnections(m, p)
         for c in conns:
            print(f"{m}:{p} -> {c[0]}:{c[1]}")

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
   t = _vistle.getParameterType(id, name)
   if t == "String":
      return f"'{_vistle.getEscapedStringParam(id, name)}'"
   elif t == "Vector":
      v = _vistle.getVectorParam(id, name)
      return ', '.join(str(c) for c in v)
   elif t == "IntVector":
      v = _vistle.getIntVectorParam(id, name)
      return ', '.join(str(c) for c in v)
   else:
      return getParameter(id, name)

def showParameter(id, name):
   print(getParameter(id, name))

def showParameters(id):
   params = _vistle.getParameters(id)
   print("name\ttype\tvalue")
   for p in params:
      print(f"{p}\t{_vistle.getParameterType(id, p)}\t{getParameter(id, p)}")

def showAllParameters():
   mods = _vistle.getRunning()
   mods.insert(0, _vistle.Id.Vistle)
   mods.insert(0, _vistle.Id.Config)
   print("id\tmodule\tname\ttype\tvalue")
   for m in mods:
      name = _vistle.getModuleName(m)
      params = _vistle.getParameters(m)
      for p in params:
          print(f"{m}\t{name}\t{p}\t{_vistle.getParameterType(m, p)}\t{getSavableParam(m, p)}")

def modvar(id):
   if (id >= 0):
      return f"m{_vistle.getModuleName(id)}{id}"
   if (id == _vistle.getVistleSession()):
       return "VistleSession"
   if (id == _vistle.getWorkflowConfig()):
       return "WorkflowConfig"
   return f"v{-id}"

def hubVar(hub, numSlaves):
   if (hub == _vistle.getMasterHub()):
      return "MasterHub"
   if (numSlaves == 1):
      return "SlaveHub"
   return f"Slave{-hub}"

def hubVarForModule(id, numSlaves):
   hub = _vistle.getHub(id)
   return hubVar(hub, numSlaves)

def isModuleParameter(name):
   m = re.search(r'\[([0-9]+)\]$', name)
   if m == None:
      return None
   return int(m.group(1))

def adaptModuleParameter(name, oldIdx, newIdx):
   m = re.search(r'\[([0-9]+)\]$', name)
   return re.sub(f'\\[{oldIdx}\\]$', f'[{newIdx}]', name)

def saveParameters(f, mod):
      ses = _vistle.getVistleSession()
      params = _vistle.getParameters(ses)
      for p in params:
         if isModuleParameter(p) == mod:
            if not _vistle.isParameterDefault(ses, p):
               np = adaptModuleParameter(p, mod, f"{{{modvar(mod)}}}")
               f.write(f"set{getParameterType(ses,p)}Param({modvar(ses)}, f'{np}', {getSavableParam(ses,p)})\n")
      params = _vistle.getParameters(mod)
      paramChanges = False
      for p in params:
         if mod != ses or isModuleParameter(p) == None:
            if not _vistle.isParameterDefault(mod, p):
               f.write(f"set{getParameterType(mod,p)}Param({modvar(mod)}, '{p}', {getSavableParam(mod,p)}, True)\n")
               paramChanges = True
      if paramChanges:
         f.write(f"applyParameters({modvar(mod)})\n")
      f.write("\n")

def saveWorkflow(f, mods, numSlaves, remote):
   f.write("\n")

   if remote:
      f.write("# spawn all remote modules\n")
   else:
      f.write("# spawn all local modules\n")
   for m in mods:
      hub = _vistle.getHub(m)
      if (remote and hub==_vistle.getMasterHub()) or (not remote and hub!=_vistle.getMasterHub()):
            continue
      #f.write(modvar(m)+" = spawn('"+_vistle.getModuleName(m)+"')\n")
      f.write(f"u{modvar(m)} = spawnAsync({hubVarForModule(m, numSlaves)}, '{_vistle.getModuleName(m)}')\n")
   f.write("\n")

   for m in mods:
      hub = _vistle.getHub(m)
      if (remote and hub==_vistle.getMasterHub()) or (not remote and hub!=_vistle.getMasterHub()):
            continue
      f.write(f"{modvar(m)} = waitForSpawn(u{modvar(m)})\n")
      dm = _vistle.getModuleDisplayName(m)
      if dm != "" and dm != _vistle.getModuleName(m):
         f.write(f"setModuleDisplayName({modvar(m)}, '{dm}')\n")
      saveParameters(f, m)

   if remote:
      f.write("# connections between local and remote\n")
   else:
      f.write("# all local connections\n")
   writtenConns = []
   for m in mods:
      hub = _vistle.getHub(m)
      if (not remote and hub!=_vistle.getMasterHub()):
            continue
      ports = _vistle.getOutputPorts(m)
      params = _vistle.getParameters(m)
      all = ports + params
      for p in all:
         conns = _vistle.getConnections(m, p)
         for c in conns:
            hub2 = _vistle.getHub(c[0])
            if (remote and hub==_vistle.getMasterHub() and hub2==_vistle.getMasterHub()) or (not remote and (hub!=_vistle.getMasterHub() or hub2!=_vistle.getMasterHub())):
               continue
            if [str(m), p] in writtenConns:
               continue
            else:
               f.write(f"connect({modvar(m)},'{p}', {modvar(c[0])},'{c[1]}')\n")
               writtenConns.append([str(c[0]), c[1]])



def save2(file, moduleList):
   file.write("# /usr/bin/env vistle\n")
   file.write(f"# this is a Python workflow from Vistle {_vistle.version()}\n")
   file.write("\n")

   file.write("MasterHub=getMasterHub()\n")
   file.write("VistleSession=getVistleSession()\n")
   file.write("WorkflowConfig=getWorkflowConfig()\n")

   master = getMasterHub()
   slavehubs = set()
   for m in moduleList:
      h = _vistle.getHub(m)
      if h != master:
         slavehubs.add(h)
   numSlaves = len(slavehubs)

   file.write("uuids = {}\n");

   saveParameters(file, getVistleSession())
   saveParameters(file, getWorkflowConfig())
   saveWorkflow(file, moduleList, numSlaves, False)

   if numSlaves > 1:
      print(f"slave hubs: {slavehubs}")
      file.write(f"slavehubs = waitForHubs({numSlaves})\n")
      count = 0
      for h in slavehubs:
          file.write(f"{hubVar(h, numSlaves)} = slavehubs[{count}]\n")
          count = count+1
   elif numSlaves > 0:
      print(f"slave hubs: {slavehubs}")
      file.write("print('waiting for a slave hub to connect...')\n")
      file.write("printInfo('waiting for a slave hub to connect...')\n")
      file.write("SlaveHub=waitForHub()\n")
      file.write("print(f'slave hub {SlaveHub} connected\\n')\n")
      file.write("printInfo(f'slave hub {SlaveHub} connected\\n')\n")

   saveWorkflow(file, moduleList, numSlaves, True)



def save(filename = None):
   if filename == None:
      filename = _vistle.getLoadedFile()
   if filename == None or filename == "":
      print("No file loaded and no file specified")
      return

   _vistle.setLoadedFile(filename)

   f = open(f"{filename}.new", 'w')
   mods = _vistle.getRunning()

   save2(f, mods)

   #f.write("checkMessageQueue()\n")

   f.close()
   os.replace(f"{filename}.new", filename)
   print(f"Workflow saved to {filename}")

def reset():
   mods = _vistle.getRunning()
   for m in mods:
      _vistle.kill(m)
   _vistle.barrier("reset, automatic")
   #_vistle._resetModuleCounter()
   _vistle.setLoadedFile("")
   _vistle.setStatus("Workflow cleared")

def load(filename = None):
   if filename == None:
      filename = _vistle.getLoadedFile()
   if filename == None or filename == "":
      print("File name required")
      return

   reset()

   _vistle.source(filename)

   _vistle.setLoadedFile(filename)
   _vistle.setStatus(f"Workflow loaded: {filename}")

def snapshotCover(modId, file):
    global coGRMsgLoaded
    global coGRMsg
    if (not coGRMsgLoaded):
        try:
            import coGRMsg
            coGRMsgLoaded = True
        except ImportError:
            print("import of coGRMsg failed")
    if (coGRMsgLoaded):
        msg = coGRMsg.coGRPluginMsg("PBufferSnapShot", "load")
        sendCoverMessage(msg, modId)
        if not file.endswith(".png"):
         printWarning("snapshotCover: file should end with \".png\"")
        msg = coGRMsg.coGRSnapshotMsg(file, "snapOnce")
        sendCoverMessage(msg, modId)

def snapshotGui(file, quitAfter = False):
   _vistle.snapshotGui(file, quitAfter)

class PythonStateObserver(_vistle.StateObserver):
    def __init__(self, verbose = False):
        super(PythonStateObserver, self).__init__(verbose)
    def moduleAvailable(self, hub, name, path):
        super(PythonStateObserver, self).moduleAvailable(hub, name, path)
    def newModule(self, moduleId, spawnUuid, moduleName):
        super(PythonStateObserver, self).newModule(moduleId, spawnUuid, moduleName)
    def deleteModule(self, moduleId):
        super(PythonStateObserver, self).deleteModule(moduleId)
    def moduleStateChanged(self, moduleId, stateBits):
        super(PythonStateObserver, self).moduleStateChanged(moduleId, stateBits)
    def newParameter(self, moduleId, parameterName):
        super(PythonStateObserver, self).newParameter(moduleId, parameterName)
    def deleteParameter(self, moduleId, parameterName):
        super(PythonStateObserver, self).deleteParameter(moduleId, parameterName)
    def parameterValueChanged(self, moduleId, parameterName):
        super(PythonStateObserver, self).parameterValueChanged(moduleId, parameterName)
    def parameterChoicesChanged(self, moduleId, parameterName):
        super(PythonStateObserver, self).parameterChoicesChanged(moduleId, parameterName)
    def newPort(self, moduleId, portName):
        super(PythonStateObserver, self).newPort(moduleId, portName)
    def deletePort(self, moduleId, portName):
        super(PythonStateObserver, self).deletePort(moduleId, portName)
    def newConnection(self, fromId, fromName, toId, toName):
        super(PythonStateObserver, self).newConnection(fromId, fromName, toId, toName)
    def deleteConnection(self, fromId, fromName, toId, toName):
        super(PythonStateObserver, self).deleteConnection(fromId, fromName, toId, toName)
    def info(self, text, textType, senderId, senderRank, refType, refUuid):
        super(PythonStateObserver, self).info(text, textType, senderId, senderRank, refType, refUuid)
    def status(self, moduleId, text, prio):
        super(PythonStateObserver, self).status(moduleId, text, prio)
    def itemInfo(self, text, type, senderId, port):
        super(PythonStateObserver, self).itemInfo(text, type, senderId, port)
    def updateStatus(self, moduleId, text, prio):
        super(PythonStateObserver, self).updateStatus(moduleId, text, prio)
    def setName(self, moduleId, name):
        super(PythonStateObserver, self).setName(moduleId, name)


# re-export functions from _vistle
version = _vistle.version
source = _vistle.source
removeHub = _vistle.removeHub
spawn = _vistle.spawn
spawnAsync = _vistle.spawnAsync
migrateAsync = _vistle.migrateAsync
migrate = _vistle.migrate
replace = _vistle.replace
restart = _vistle.restart
waitForSpawn = _vistle.waitForSpawn
kill = _vistle.kill
connect = _vistle.connect
disconnect = _vistle.disconnect
compute = _vistle.compute #execute called compute to prevent name clash with python execute
interrupt = _vistle.interrupt
quit = _vistle.quit
trace = _vistle.trace
debug = _vistle.debug
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
setModuleDisplayName = _vistle.setModuleDisplayName
getModuleDisplayName = _vistle.getModuleDisplayName
setParam = _vistle.setParam
setIntParam = _vistle.setIntParam
setFloatParam = _vistle.setFloatParam
setStringParam = _vistle.setStringParam
setVectorParam = _vistle.setVectorParam
setIntVectorParam = _vistle.setIntVectorParam
applyParameters = _vistle.applyParameters
getAvailable = _vistle.getAvailable
getModuleCategory = _vistle.getModuleCategory
getModuleDescription = _vistle.getModuleDescription
getRunning = _vistle.getRunning
getBusy = _vistle.getBusy
getModuleName = _vistle.getModuleName
hubName = _vistle.hubName
waitForHub = _vistle.waitForHub
waitForHubs = _vistle.waitForHubs
waitForNamedHubs = _vistle.waitForNamedHubs
getMasterHub = _vistle.getMasterHub
getVistleSession = _vistle.getVistleSession
getWorkflowConfig = _vistle.getWorkflowConfig
getSessionUrl = _vistle.getSessionUrl
getAllHubs = _vistle.getAllHubs
getHub = _vistle.getHub
getPortDescription = _vistle.getPortDescription
getInputPorts = _vistle.getInputPorts
getOutputPorts = _vistle.getOutputPorts
getConnections = _vistle.getConnections
getParameters = _vistle.getParameters
getParameterGroup = _vistle.getParameterGroup
getParameterType = _vistle.getParameterType
getParameterPresentation = _vistle.getParameterPresentation
getParameterTooltip = _vistle.getParameterTooltip
getParameterChoices = _vistle.getParameterChoices
isParameterDefault = _vistle.isParameterDefault
getIntParam = _vistle.getIntParam
getFloatParam = _vistle.getFloatParam
getVectorParam = _vistle.getVectorParam
getIntVectorParam = _vistle.getIntVectorParam
getStringParam = _vistle.getStringParam
getEscapedStringParam = _vistle.getEscapedStringParam
Id = _vistle.Id
Message = _vistle.Message
StateObserver = _vistle.StateObserver
Text = _vistle.Text
Status = _vistle.Status
Item = _vistle.Item

moduleCompoundAlloc = _vistle.moduleCompoundAlloc
moduleCompoundAddModule = _vistle.moduleCompoundAddModule
moduleCompoundConnect = _vistle.moduleCompoundConnect
moduleCompoundCreate =_vistle.moduleCompoundCreate
setRelativePos = _vistle.setRelativePos
setCompoundDropPosition = _vistle.setCompoundDropPosition
