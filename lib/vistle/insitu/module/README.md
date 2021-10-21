InSituReader
=====================================================
Base class for modules that connect to external processes(e.g simulations) and are continuously reading data during execute.
After beginExecute() is called the module must not create vistle objects until endExecute() returns.
Vice versa the external process must create its vistle objects after beginExecute() and before endExecute() returns.
Use insitu::message::SyncShmIDs::createVistleObject (initialized with with instance = InstanceNum() of this module) to create vistle objects in the external process. 
Use a vistle::message_queue with name "recvFromSim" + InstanceNum() in the external process to send the vistle objects to this module which will send them with its own signature to the manager.

 
