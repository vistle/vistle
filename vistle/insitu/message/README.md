In-situ message
=====================================================
Contains classes to exchange messages between Vistle and simulation-adapters.

There are two ways of data exchange supported: tcp and shared memory, with the classes InSituTcpMessage and InSituShmMessage.
Both message classes must be initialized once after connection and are then able to send and receive messages. 

The InSituTcpMessage is designed to only use a tcp connection on rank 0 and then broadcast the results to the other ranks.
Therefore it is nececarry for the receive method to be called on all ranks consistently.

The InSituShmMessage sends and receives on all ranks independently.

The SyncShmIDs class provides utility to synchronize the shm-object ids of vistle objects. Use the createVistleObject function 
in the simulation adapters to make sure the object ids are synced. In single process mode this class is only a dummy.
