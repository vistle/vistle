Vistle Documentation
********************
.. important::
   The Vistle documentation has been moved to https://vistle.io/. 
   Please note that the ReadTheDocs documentation here will no longer be updated.


Vistle is an extensible software environment that integrates simulations on supercomputers, post-processing and parallel interactive visualization in immersive virtual environments.

It is under active development at HLRS since 2012. The objective is to provide a highly scalable visualization tool, exploiting data, task and pipeline parallelism in hybrid shared and distributed memory environments with acceleration hardware. Domain decompositions used during simulation can be reused for visualization.

A Vistle work flow consists of several processing modules, each of which is a parallel MPI program that uses OpenMP within nodes. These can be configured graphically or from Python. Shared memory is used for transfering data between modules on a single node. Work flows can be distributed across several clusters.

The Vistle system is modular and can be extended easily with additional visualization algorithms. Source code is available on `GitHub <https://github.com/vistle/vistle>`__ and licensed under the LPGL.

More infos about Architecture: https://vistle.io/architecture/

..
   * :ref:`build-docs`
   * :ref:`user-docs`
   * :ref:`dev-docs`

.. _build-docs:

.. toctree::
   :maxdepth: 1
   :caption: Build Documentation
    
   Build Instructions <build/index.rst> 

.. _user-docs:

.. toctree::
   :maxdepth: 1
   :caption: User Documentation

   Quickstart <quickstart/quickstart.md>
   Introduction <intro/index.rst>
   Module Guide <modules/index.rst>
   Library Documentation <lib/index.rst>

.. _dev-docs:

.. toctree::
   :maxdepth: 1
   :caption: Developer Documentation

   Application <app/index.rst>
   Coding <dev/index.rst>
   Documentation <dev/documentation.rst>
