import sys;
import _vistle;
from _vistle import *;

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
#sys.stdin = _stdin()

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
