How to Write Documentation
==========================
The documentation is written in markdown (.md) files. The root directory is 
`vistle/doc`. Use the `index.rst` files in each directory to include your markdown files.
Links to other markdown files can be set with relative paths: 
```
[name](../pathToName.md)
```
Links to Vistle modules will be automatically generated for keys with the syntax:
```
[ModuleName]()
```
Additional sources from the source tree that are needed for the documentation, e.g. README files or pictures have to be presented to the  
documentation build proces via cmake, e.g.:
    
    vistle_install_docs(README.md test.png)

These files are copied to the documentation directory under docs/readme.

Documentation for modules is handled specially: their source markdown files must reside in the source tree module directory and be named after the module it describes (vistle/module/category/modulename.md). The module documentation can contain special tags that are replaced when the documentation is built:

    [headline]:<>   : the module name and tooltip description
    [moduleHtml]:<> : graphical representation of the modules input and output ports
    [parameters]:<> : list of parameters and their tooltips

Use them to structure the module documentation. If they are omitted, the headline will be inserted at the top of the document and moduleHtml and parameters are appended at the end.

Additionally there should be at least one example presented that shows how the module is used. To automatically generate an example use the tag     
    
    [example]:<example_net> : generate images of the workflow and the result


where example_net is a checked in .vsl int the module source directory.
Precede this tag with a custom description of the example.

As orientation you can work with a [template](path:module-doc-template.md):


    $<vistle/modules/category/yourmodule> cp .../vistle/doc/develop/documentation/module-doc-template.md yourmodule.md

Additional data or pictures that are referenced in yourmodule.md via

    [](some_file)

are automatically copied into the documentation build.


How to Build Documentation for Read the Docs
============================================

Install Sphinx by navigating to `.../vistle/doc/build/readthedocs` and type in


    $ pip install -r requirements.txt

You need to compile Vistle first to be able to build the documentation. Follow the instructions in the top level [README.md](https://github.com/vistle/vistle?tab=readme-ov-file#installation) for the compilation.

After compiling Vistle type in the following commands:

    
    $<Path/to/build> cmake -DVISTLE_DOCUMENTATION_DIR="path/to/documentation --build . --target vistle_doc
