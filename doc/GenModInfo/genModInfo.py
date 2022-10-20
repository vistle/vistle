import os
import re
import subprocess
import vistle

#-------------Constants-------------#
# these have to be set to chose the module to create documentation for
# the directory containing the readTheDocs module files
BUILDDIR = os.environ['VISTLE_DOCUMENTATION_BIN_DIR']
SOURCEDIR = os.environ['VISTLE_DOCUMENTATION_DIR']
TARGET = os.environ['VISTLE_DOCUMENTATION_TARGET']
DESTDIR = os.path.dirname(os.path.realpath(
    __file__)) + "/moduleDescriptions/"

REG_IMAGE = re.compile(r"(?:\w+(?:-\w+)+|\w*)\.(?:jpg|gif|png|bmp)")
REG_TAGS_TMPL = r"\[{tag}\]:<(\w*)>"

HEADLINE_TAG = "headline"
HTML_TAG = "moduleHtml"
PARAM_TAG = "parameters"
EXAMPLE_TAG = "example"
VSL_TAG = "vslFile"

#-------------Functions-------------#


def createModuleImage(name, inPorts, outPorts):
    size = 1.
    letterWidth = 58 / 100 * size
    widths = [len(name) * letterWidth + size, size *
              (len(inPorts) + 2), size * (len(outPorts) + 2)]
    width = max(widths)
    height = 3*size
    spiderLegOffsetTop = size * len(inPorts) + size*0.8
    spiderLegOffsetBot = size * len(outPorts) + size*0.8
    image = "<svg width=\"" + str(10 * width) + "em\" height=\"" + str(
        height + spiderLegOffsetTop + spiderLegOffsetBot) + "em\" >\n"
    image += getStyle(size) + "\n"
    image += getRect(0, spiderLegOffsetTop, width,
                     height, "#64c8c8ff", 0.1, 0.1) + "\n"

    portDistance = size/5
    portStartPos = portDistance
    num = len(inPorts)
    for port, tooltip in inPorts:
        image += getRect(portStartPos, spiderLegOffsetTop, size,
                         size, color="#c81e1eff", tooltip=port) + "\n"
        text = tooltip + "<tspan> (" + port + ")</tspan>"
        image += getPortDescriptionSpider(size, portStartPos +
                                          size/2, spiderLegOffsetTop, num, text) + "\n"
        portStartPos += portDistance + size
        num -= 1

    image += getText(name, portDistance, spiderLegOffsetTop +
                     1.85*size, "moduleName")

    num = -1 * len(outPorts)
    portStartPos = portDistance
    for port, tooltip in outPorts:
        image += getRect(portStartPos, spiderLegOffsetTop + height -
                         size, size, size, color="#c8c81eff", tooltip=port) + "\n"
        text = tooltip + "<tspan> (" + port + ")</tspan>"
        image += getPortDescriptionSpider(size, portStartPos +
                                          size/2, spiderLegOffsetTop + height, num, text) + "\n"
        portStartPos += portDistance + size
        num += 1
    image += "</svg>"
    return image


def getPortDescriptionSpider(size, x, y, num, desc):
    width = size/30
    height = abs(size * num)
    f1 = 1
    f2 = 1
    if num < 0:
        f1 = 0
        f2 = -1
    spider = getRect(x, y - f1 * height, width, height, "#000000") + "\n"
    spider += getRect(x, y - f2 * height, size, width, "#000000") + "\n"
    spider += getText(desc, x + 1.2 * size, y - f2 * height + size/10, "text")
    return spider


def getStyle(size):
    style = "<style>"
    style += ".text { font: normal " + str(size) + "em sans-serif;}"
    style += "tspan{ font: italic " + str(size) + "em sans-serif;}"
    style += ".moduleName{ font: bold " + str(1.0 * size) + "em sans-serif;}"
    style += "</style>"
    return style


def getText(text, x, y, style):
    return "<text x=\"" + str(x) + "em\" y=\"" + str(y) + "em\" class=\"" + style + "\" >" + text + "</text>"


def getRectTooltip(tooltip):
    return "<title>" + tooltip + "</title></rect>"


def getRect(x, y, width, height, color, rx=0.0, ry=0.0, tooltip=""):
    rect = "<rect "
    rect += "x=\"" + str(x) + "em\" "
    rect += "y=\"" + str(y) + "em\" "
    rect += "width=\"" + str(width) + "em\" "
    rect += "height=\"" + str(height) + "em\" "
    rect += "rx=\"" + str(rx) + "em\" "
    rect += "ry=\"" + str(ry) + "em\" "
    rect += "style=\"fill:" + color + ";\""
    if len(tooltip) > 0:
        rect += " >\n"
        rect += getRectTooltip(tooltip)
    else:
        rect += " />"
    return rect


def getHeadline(line, typestr):
    return "" if len(line) == 0 else typestr + " " + line + '\n'


def getTableLine(entries):
    retval = "|"
    for entry in entries:
        retval += entry + "|"
    retval += "\n"
    return retval


def getComment(comment):
    return "[" + comment + "]:<>"


def getModuleHeadLine(mod):
    name = vistle.getModuleName(mod)
    desc = vistle.getModuleDescription(mod)
    return "\n" + getHeadline(name, "#") + desc + "\n"


def getModuleHtml(mod):
    inPorts = []
    for p in vistle.getInputPorts(mod):
        inPorts.append([p, vistle.getPortDescription(mod, p)])
    outPorts = []
    for p in vistle.getOutputPorts(mod):
        outPorts.append([p, vistle.getPortDescription(mod, p)])
    return "\n" + createModuleImage(vistle.getModuleName(mod), inPorts, outPorts) + "\n"


def getPortDescriptionString(mod, ports, headline):
    portsDescription = "\n"
    if len(ports) > 0:
        portsDescription += getHeadline(headline, "##")
        portsDescription += getTableLine(["name",
                                         "description"]) + getTableLine(["-", "-"])
    for port in ports:
        portsDescription += getTableLine([port,
                                         vistle.getPortDescription(mod, port)])
    portsDescription += "\n"
    return portsDescription


def getInputPortsStringDescription(mod):
    return getPortDescriptionString(mod, vistle.getInputPorts(mod), "Input ports")


def getOutputPortsDescription(mod):
    return getPortDescriptionString(mod, vistle.getOutputPorts(mod), "Output ports")


def getParametersString(mod):
    allParams = vistle.getParameters(mod)
    params = []
    for param in allParams:
        if param[0] != '_':
            params.append(param)
    paramString = "\n"
    if len(params) > 0:
        paramString += getHeadline("Parameters", "##") + getTableLine(
            ["name", "description", "type"]) + getTableLine(["-", "-", "-"])
    for param in params:
        paramString += getTableLine([param, vistle.getParameterTooltip(
            mod, param), vistle.getParameterType(mod, param)])
    return paramString


def getExampleString(mod, example) -> str:
    line = f"""<figure float=\"left\">
    <img src=\"{example}_workflow.png\" width=\"200\"/>
    <img src=\"{example}_result.png\" width=\"300\"/>
    <figcaption>Fig.1 {example} workflow (left) and expected result (right).</figcaption>
</figure>"""

    return line


def genVistleScreenshot(mod, line) -> str:
    line_start_with_tag = re.search(
        r"^" + REG_TAGS_TMPL.format(tag=VSL_TAG), line)
    vsl_name_without_ext = re.search('<(.+?)>', line)
    out = ""
    if vsl_name_without_ext:
        modulename = vistle.getModuleName(mod)
        vsl_name = vsl_name_without_ext.group(1)
        vsl_dir = SOURCEDIR + "/" + vsl_name
        img_path = vsl_dir + '.png'
        vsl_path = vsl_dir + '.vsl'

        if os.path.exists(vsl_path):
            if line_start_with_tag:
                out += "![](" + vsl_name + ".png) \n"
            else:
                out += re.sub(REG_TAGS_TMPL.format(tag=VSL_TAG),
                              vsl_name + ".png", line)
            subprocess.run(
                ["vistle", "--snapshot", img_path, vsl_path], check=True)
    return out


def readAdditionalDocumentation(filename):
    content = ""
    if os.path.isfile(filename):
        with open(filename, "r") as f:
            content = f.readlines()
    return content


def relinkImage(line: str, sourceDir, destDir):
    for match in re.finditer(REG_IMAGE, line):
        relPath = os.path.relpath(sourceDir, destDir) + "/" + match.group()
        line = line.replace(match.group(), relPath)
    return line


def findImage(line):
    match = re.search(REG_IMAGE, line)
    if match:
        return ("image", match.group())
    return (None, None)


def findTag(line, tags):
    """
    Returns tuple with tag and its value.

    Args:
        line: current line in file
        tags: tag list
    """

    for tag in tags:
        matches = re.search(REG_TAGS_TMPL.format(tag=tag), line)
        if matches:
            return (tag, matches.group(1)) if tag is not VSL_TAG else (tag, line)
    return (None, None)


def generateModuleDescriptions() -> None:
    filename = SOURCEDIR + "/" + TARGET + ".md"
    # tags with the format [tag]:<value> can be replace by asignig a replacement function to the tag:
    essential_tags = {  # these can only apear once and if not found these will be added at the end
        HEADLINE_TAG: getModuleHeadLine,
        HTML_TAG: getModuleHtml,
        PARAM_TAG: getParametersString
    }

    optional_tags = {
        VSL_TAG: genVistleScreenshot,
        EXAMPLE_TAG: getExampleString
    }

    # spawn the module and wait for its information to arrive at the hub
    mod = vistle.spawn(TARGET)
    vistle.barrier()

    content_list = readAdditionalDocumentation(filename)
    output = []
    used_tags = set()
    for line in content_list:
        tag, _ = findTag(line, essential_tags.keys())
        # check if its an essential tag
        if tag is not None:
            line = essential_tags[tag](mod)
            used_tags.add(tag)
        else:
            tag, value = findTag(line, optional_tags.keys())
            # check if its an optional tag
            if tag is not None:
                line = optional_tags[tag](mod, value)
        source = SOURCEDIR
        dest = DESTDIR
        if tag is EXAMPLE_TAG:
            source = BUILDDIR
        line = relinkImage(line, source, dest)
        output.append(line)

    # add the missing essential sections
    with open(DESTDIR + TARGET + ".md", "w") as f:
        unused_essential_tags = essential_tags.keys() - used_tags
        if HEADLINE_TAG in unused_essential_tags:
            f.write(essential_tags[HEADLINE_TAG](mod))
            unused_essential_tags.remove(HEADLINE_TAG)
        # add content to file
        _ = [f.write(line) for line in output]
        # add default for unused essential keys
        _ = [f.write(essential_tags[tag](mod))
             for tag in sorted(unused_essential_tags)]

    vistle.quit()


#-------------Main (ONLY FOR TESTING) -------------#
if __name__ == "__main__":
    # use main for testing your functions
    tag_functions = {
        HEADLINE_TAG: getModuleHeadLine,
        HTML_TAG: getModuleHtml,
        PARAM_TAG: getParametersString,
        VSL_TAG: genVistleScreenshot
    }
    vistleDir = os.path.dirname(os.path.realpath(__file__)) + "/../../"
    sourceDir = vistleDir + "module/read/ReadTsunami/"
    source = sourceDir + "ReadTsunami.md"
    destDir = os.path.dirname(os.path.realpath(
        __file__)) + "/moduleDescriptions/"
    contentList = readAdditionalDocumentation(source)
    searchTags = tag_functions.keys()
    # tagPos = findTags(contentList, tag_functions.keys())

    # for tag, posList in tagPos.items():
    # for pos in posList:
    # line = contentList[pos]
    # if tag == VSL_TAG:
    # line = getExamples(line)
    # exampleName = re.search('<(.+?)>',line)
    # line = relinkImage(line, sourceDir, destDir)
    # print(line)

    # for unused in (tag_functions.keys() - tagPos.keys()):
    #     print(unused)
