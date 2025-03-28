from sphinx.transforms.post_transforms import SphinxPostTransform
from sphinx.application import Sphinx
from docutils import nodes
import os
import shutil
import re

class HtmlImageProcessor(SphinxPostTransform):
    default_priority = 200  # Run after other transforms

    def run(self, **kwargs):
        for node in self.document.traverse(nodes.raw):
            dirname = os.path.dirname(node.source)
            dirs = dirname.split(os.sep)
            kind = dirs[-3]
            if kind != 'module':
                continue

            if 'html' in node.get('format', ''):
                # Extract <img> tags from raw HTML
                new_html = node.astext()
                img_tags = self.extract_img_tags(new_html)
                for img_tag in img_tags:
                    src = self.extract_src(img_tag)
                    if src and not src.startswith(('http://', 'https://', '/')):
                        # Copy the image to the _images directory
                        new_src = self.copy_image(os.path.dirname(node.source), src)
                        # Replace the src in the HTML
                        # new_img_tag = img_tag.replace(src, new_src)
                        # new_html = new_html.replace(img_tag, new_img_tag)
                # Update the node's raw content
                node.rawsource = new_html
                node[:] = [nodes.raw('', new_html, format='html')]

    def extract_img_tags(self, html):
        return re.findall(r'<img[^>]+>', html)

    def extract_src(self, img_tag):
        match = re.search(r'src=["\']([^"\']+)["\']', img_tag)
        return match.group(1) if match else None

    def copy_image(self, src_dir, image_name):
        dirs = src_dir.split(os.sep)
        moduleName = dirs[-1]
        category = dirs[-2]
        dest_dir = os.path.join(self.app.builder.outdir, 'module', category, moduleName)
        src_path = os.path.join(src_dir, image_name)
        os.makedirs(dest_dir, exist_ok=True)
        dest_path = os.path.join(dest_dir, image_name)
        print(f'Copying image {src_path} to {dest_path}')
        if os.path.exists(src_path):
            shutil.copy(src_path, dest_path)
        else:
            print(f'Image {image_name} not found in {src_path}')
        return f'_images/{os.path.basename(image_name)}'

def setup(app: Sphinx):
    app.add_post_transform(HtmlImageProcessor)
