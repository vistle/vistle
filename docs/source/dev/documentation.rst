Documentation
*************

* :ref:`snapshot`

.. _snapshot:

.. toctree::
   :maxdepth: 1
   :caption: How to Snapshot

   Snapshot <documentation/snapshot>

How to build this Documentation
===============================

Navigate to /path/to/vistle/root/docs/ and type in.

::

    $ pip install -r requirements.txt

You need to compile Vistle first to be able to build the documentation. Follow the instructions in :ref:`build-docs` for the compilation.

After compiling Vistle with the Ninja Generator for example type in the following commands:

::

    $<Path/to/build> ninja vistle_doc
    $<Path/to/build> cd ../docs
    $<Path/to/doc/root> make html


.. tip::
   Define an alias in your prefered shell which executes the previous commands to simplify the building process. 

   For example:

   alias vdoc "cd ~/vistle/build-linux64-ompi && ninja vistle_doc && cd ../docs && make html"
