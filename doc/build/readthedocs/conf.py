# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.

import os
import glob
import sys
sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------
project = 'Vistle'
author = 'Martin Aumüller, Marko Djuric, Dennis Grieger, Leyla Kern, Susanne Malheiros, Uwe Wössner'
author = 'the Vistle team'
project_copyright = f'2012 - %Y, {author}'

primary_domain = 'vistle'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'myst_parser',
    'sphinxcontrib.mermaid',
    'sphinx.ext.autosectionlabel',
    'html_image_processor',
]

# Tell sphinx what the primary language being documented is.
# primary_domain = 'vistle'

# Tell sphinx what the pygments highlight language should be.
# highlight_language = 'cpp'


myst_enable_extensions = [
    'colon_fence',
]
myst_all_links_external=False
myst_heading_anchors = 3

# some meta data
language = "en"
myst_html_meta = {
    "description lang=en": "metadata description",
    "description lang=fr": "description des métadonnées",
    "keywords": "Sphinx, MyST",
    "property=og:locale":  "en_US"
}

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
#html_theme = 'sphinx_wagtail_theme'
#html_theme = 'sphinx_material'
html_theme = 'sphinx_rtd_theme'
#html_theme = 'renku'
html_logo = 'readme/install/hlrs-vistle.svg'
html_title = 'Vistle - Distributed Data-parallel Scientific Visualization in VR'
html_short_title = 'Vistle'
html_base_url = "https://vistle.io"
html_show_sphinx = False
html_show_copyright = True

html_context = {
  "display_github": False, # Add 'Edit on Github' link instead of 'View page source'
  "github_user": "vistle",
  "github_repo": "vistle.github.io",
  "github_version": "main",
  "conf_py_path": "",
  "source_suffix": ""
  
}

html_theme_options = {
        'canonical_url': 'https://vistle.io',
        'nav_title': 'Vistle - Distributed Data-parallel Scientific Visualization in VR',
        'base_url': 'https://vistle.io',
        'repo_url': 'https://github.com/vistle/vistle/',
        'github_url': 'https://github.com/vistle/vistle/',
        'repo_name': 'Vistle',
        'flyout_display': 'attached',
        #'prev_next_buttons_location': 'both',
        #'logo_only': True,
        #'master_doc': True,
        'style_nav_header_background': '#20519F',

        
          # Visible levels of the global TOC; -1 means unlimited
    'globaltoc_depth': 2,
    # If False, expand all TOC entries
    'globaltoc_collapse': True,
    # If True, show hidden TOC entries
    'globaltoc_includehidden': True,

    # Set the color and the accent color
    'color_primary': '#20519F',
    'color_accent': '#C2A23C',
}

html_sidebars = {
    "**": ["logo-text.html", "globaltoc.html", "localtoc.html", "searchbox.html"]
}

# do not link to document source
#html_show_sourcelink = False

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
# html_static_path = ['_static']

#autosectionlabel_prefix_document = True
