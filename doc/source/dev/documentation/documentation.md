How to write Documentation
==========================
The documentation is written in markdown (.md) files. The root directory is 
vistle/doc/source. Use the index.rst files in each directory to include your markdown files.
Links to other markdown files can be set with relative paths: 
```
[name](../pathToName.md)
```
Links to Vistle modules will be automatically generated for keys with the syntax:
```
[ModuleName]()
```
Pictures should be checked in under 
    
    vistle/doc/source/pictures

and can be referenced by
```
![](pictureFileName.png)   
```

Documentation for modules is handled specially: their source markdown files must reside in the source tree module directory and be named after the module it describes (vistle/modules/category/modulename.md). The module documentation can contain special tags that are replaced when the documentation is built:

    [headline]:<>   : the module name and tooltip description
    [moduleHtml]:<> : graphical representation of the modules input and output ports
    [parameters]:<> : list of parameters and their tooltips

Use them to structure the module documentation. If they are omitted, the headline will be inserted at the top of the document and moduleHtml and parameters are appended at the end.

Additionally there should be at least one example presented that shows how the module is used. To automatically generate an example use the tag     
    
    [example]:<example_net> : generate images of the workflow and the result

    (der Viewpoint könnte in covconfig gespeichert werden, dann landet er im vsl file und man bräuhte keinen extra viewport)

where example_net is a checked in .vsl file under vistle/examples/example_net.vsl
Additional data required for the example should be as small as possible and checked in under .../vistle/example/data. 
(wir müssen sicherstellen das das dort auch gefunden wird)
Precede this tag with a custom description of the example.

As orientation you can work with a template:

    $<vistle/modules/category/yourmodule> cp .../vistle/doc/module-template.md yourmodule.md




How to build Documentation for Read the Docs
============================================

Install Sphinx by navigating to /path/to/vistle/doc/readthedocs and type in


    $ pip install -r requirements.txt

You need to compile Vistle first to be able to build the documentation. Follow the instructions in :ref:`build-docs` for the compilation.

After compiling Vistle type in the following commands:

    
    $<Path/to/build> cmake -DVISTLE_DOCUMENTATION_DIR="path/to/documentation --build . --target vistle_doc
