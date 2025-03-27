Vistle
******

`Vistle <https://vistle.io>`__ is an extensible software environment that integrates simulations on supercomputers, post-processing and parallel interactive visualization in immersive virtual environments.

It is under active development at `HLRS <https://hlrs.de>`__ since 2012. The objective is to provide a highly scalable visualization tool, exploiting data, task and pipeline parallelism in hybrid shared and distributed memory environments with acceleration hardware. Domain decompositions used during simulation can be reused for visualization.

A Vistle work flow consists of several processing modules, each of which is a parallel MPI program that uses OpenMP within nodes. These can be configured graphically or from Python. Shared memory is used for transfering data between modules on a single node. Work flows can be distributed across several clusters.

The Vistle system is modular and can be extended with additional visualization algorithms. Source code is available on `GitHub <https://github.com/vistle/vistle>`__ and licensed under the `LPGL (version 2 or newer) <https://spdx.org/licenses/LGPL-2.0-or-later.html>`__.

..
   * :ref:`install-docs`
   * :ref:`user-docs`
   * :ref:`dev-docs`


Installation
------------

Refer to the top level `README.md <https://github.com/vistle/vistle?tab=readme-ov-file#installation>`__ for basic installation instructions.

.. _install-docs:

.. toctree::
   :maxdepth: 1
   :caption: Overview
   :hidden:

   Installation <self>

.. _user-docs:

.. toctree::
   :maxdepth: 1
   :caption: User Documentation

   Quickstart <quickstart/quickstart.md>
   Introduction <intro/index.rst>
   Module Reference <module/index.rst>

.. _dev-docs:

.. toctree::
   :maxdepth: 1
   :caption: Developer Documentation

   Conventions & Guidelines <develop/coding-style.md>
   API Overview <develop/api/index.rst>
   Creating Modules <develop/module/index.rst>
   Write & Build Documentation <develop/documentation/index.rst>
