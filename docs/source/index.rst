Vistle Documentation
********************

Vistle is an extensible software environment that integrates simulations on supercomputers, post-processing and parallel interactive visualization in immersive virtual environments.

It is under active development at HLRS since 2012. The objective is to provide a highly scalable visualization tool, exploiting data, task and pipeline parallelism in hybrid shared and distributed memory environments with acceleration hardware. Domain decompositions used during simulation can be reused for visualization.

A Vistle work flow consists of several processing modules, each of which is a parallel MPI program that uses OpenMP within nodes. These can be configured graphically or from Python. Shared memory is used for transfering data between modules on a single node. Work flows can be distributed across several clusters.

The Vistle system is modular and can be extended easily with additional visualization algorithms. Source code is available on `GitHub <https://github.com/vistle/vistle>`__ and licensed under the LPGL.

More infos about Architecture: https://vistle.io/architecture/

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

   Module Guide <modules/index.rst>
   Library Documentation <lib/index.rst>

.. _dev-docs:

.. toctree::
   :maxdepth: 1
   :caption: Developer Documentation

   Application <app/index.rst>
   Coding <dev/index.rst>

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

Building this Documentation
===========================

Requirements:

- Sphinx

::

    $ pip install sphinx

- myst-parser

::

    $ pip install myst-parser

- sphinxcontrib-mermaid

::

    $ pip install sphinxcontrib-mermaid


Building the documentation with:

::

    $<Path/to/doc/root> make html
