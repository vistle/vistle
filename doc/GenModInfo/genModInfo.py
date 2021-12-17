import time
import os
import re
from bisect import insort
from _vistle import getModuleName, getModuleDescription, getInputPorts, getOutputPorts, getPortDescription, getParameters, getParameterTooltip, getParameterType, spawn, barrier, quit

REG_IMAGE = re.compile(r"!\[\w*\](\S*)$")
REG_PARENTHESIS = re.compile(r"\((.*?)\)")
REG_SUB_PARENTHESIS = re.compile(r"\([\s\S]*\)")
REG_TAGS_TMPL = r"\[{tag}\]:<\w*>$"

#-------------Functions-------------#
def createModuleImage(name, inPorts, outPorts):
    size = 30
    letterWidth = 58 / 100 * size
    widths = [len(name) * letterWidth + size, size * (len(inPorts) + 2), size * (len(outPorts) + 2) ]
    width = max(widths)
    height = 3*size
    spiderLegOffsetTop = size * len(inPorts) + size
    spiderLegOffsetBot = size * len(outPorts) + size
    image = "<svg width=\"" + str(10 * width) + "\" height=\"" + str(height +spiderLegOffsetTop + spiderLegOffsetBot) + "\" >\n"
    image += getStyle(size) + "\n"
    image += getRect(0, spiderLegOffsetTop, width, height, "#64c8c8ff", 5, 5) + "\n"

    portDistance = size/5
    portStartPos = portDistance
    num = len(inPorts)
    for port, tooltip in inPorts:
        image += getRect(portStartPos, spiderLegOffsetTop, size , size, color="#c81e1eff", tooltip=port) + "\n"
        text = tooltip + "<tspan> ("+ port + ")</tspan>"
        image += getPortDescriptionSpider(size, portStartPos + size/2, spiderLegOffsetTop, num, text) + "\n"
        portStartPos += portDistance + size
        num -= 1

    num = -1 * len(outPorts)
    portStartPos = portDistance
    for port, tooltip in outPorts:
        image += getRect(portStartPos, spiderLegOffsetTop + height - size, size, size, color="#c8c81eff", tooltip=port) + "\n"
        text = tooltip + "<tspan> ("+ port + ")</tspan>"
        image += getPortDescriptionSpider(size, portStartPos + size/2, spiderLegOffsetTop + height, num, text) + "\n"
        portStartPos += portDistance + size
        num += 1
    image += getText(name, portDistance, spiderLegOffsetTop + 1.85*size, "moduleName")
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
    spider += getRect(x, y - f2* height, size, width, "#000000") + "\n"
    spider += getText(desc, x + 1.2 * size, y - f2 * height + size/10, "text")
    return spider

def getStyle(size):
    style = "<style>"
    style += ".text { font: normal " + str( 0.8 * size) + "px sans-serif;}"
    style += "tspan{ font: italic " + str(0.8 * size) + "px sans-serif;}"
    style += ".moduleName{ font: italic " + str(size) + "px sans-serif;}"
    style += "</style>"
    return style


def getText(text, x, y, style):
    return "<text x=\"" + str(x) + "\" y=\"" + str(y) + "\" class=\"" + style + "\" >" + text + "</text>"


def getRectTooltip(tooltip):
    return "<title>" + tooltip + "</title></rect>"


def getRect(x, y , width, height, color, rx = 0, ry = 0, tooltip = ""):
    rect = "<rect "
    rect += "x=\"" + str(x) + "\" "
    rect += "y=\"" + str(y) + "\" "
    rect += "width=\"" + str(width) + "\" "
    rect += "height=\"" + str(height) + "\" "
    rect += "rx=\"" + str(rx) + "\" "
    rect += "ry=\"" + str(ry) + "\" "
    rect += "style=\"fill:" + color + ";\""
    if len(tooltip) > 0:
        rect += " >\n"
        rect +=  getRectTooltip(tooltip)
    else:
        rect += " />"
    return rect


def getHeadline(line, type):
    return "" if len(line) == 0 else type + " " + line + '\n'


def getTableLine(entries):
    retval = "|"
    for entry in entries:
        retval += entry + "|"
    retval += "\n"
    return retval


def getComment(comment):
    return "[" + comment + "]:<>"


def getModuleHeadLine(mod):
    name = getModuleName(mod)
    desc = getModuleDescription(mod)
    return "\n" + getHeadline(name, "#")  + desc.capitalize() + "\n"


def getModuleHtml(mod):

    inPorts = []
    for p in getInputPorts(mod):
        inPorts.append([p, getPortDescription(mod, p)])
    outPorts = []
    for p in getOutputPorts(mod):
        outPorts.append([p, getPortDescription(mod, p)])
    return  "\n" + createModuleImage(getModuleName(mod), inPorts, outPorts) + "\n"


def getPortDescriptionString(mod, ports, headline):
    portsDescription = "\n"
    if len(ports) > 0:
        portsDescription += getHeadline(headline, "##")
        portsDescription += getTableLine(["name", "description"]) + getTableLine(["-", "-"])
    for port in ports:
        portsDescription += getTableLine([port,getPortDescription(mod, port)])
    portsDescription += "\n"
    return portsDescription


def getInputPortsStringDescription(mod):
    return getPortDescriptionString(mod, getInputPorts(mod), "Input ports")


def getOutputPortsDescription(mod):
    return getPortDescriptionString(mod, getOutputPorts(mod), "Output ports")


def getParametersString(mod):
    allParams = getParameters(mod)
    params = []
    for param in allParams:
        if param[0] != '_':
            params.append(param)
    paramString = "\n"
    if len(params) > 0:
        paramString += getHeadline("Parameters", "##") + getTableLine(["name", "description", "type"]) + getTableLine(["-", "-", "-"])
    for param in params:
        paramString += getTableLine([param, getParameterTooltip(mod, param), getParameterType(mod, param)])
    return paramString


def readAdditionalDocumentation(filename):
    content = ""
    if os.path.isfile(filename):
        with open(filename, "r") as f:
            content = f.readlines()
    return content


def relinkImages(lineStr, sourceDir, destDir):
    imageName = re.search(REG_PARENTHESIS, lineStr)
    if imageName:
        relPath = os.path.relpath(sourceDir, destDir) + "/" + imageName.group(1)
        return re.sub(REG_SUB_PARENTHESIS, "(" + relPath + ")", lineStr)


def findTagPositions(lines, tags) -> {}:
    idx = 0
    tagPos = {}
    for line in lines:
        tag = isTag(line, tags)
        if tag != None:
            if tag in tagPos.keys():
                tagPos[idx] = idx
            else:
                tagPos[tag] = idx
        idx += 1
    return tagPos


def isTag(line, tags) -> str:
    for tag in tags:
        regTag = re.compile(REG_TAGS_TMPL.format(tag=tag))
        if regTag.match(line):
            return tag
        if REG_IMAGE.match(line):
            return line[line.find("[") + 1: line.find("]")]
    return None


def generateModuleDescriptions():
    #these have to be set to chose the module to create documentation for
    target = os.environ['VISTLE_DOCUMENTATION_TARGET']
    sourceDir = os.environ['VISTLE_DOCUMENTATION_DIR']
    destDir = os.path.dirname(os.path.realpath(__file__)) + "/moduleDescriptions/"
    filename = sourceDir +  "/" + target + ".md"
    tag_functions = {
        "headline": getModuleHeadLine,
        # "inputPorts": getInputPortsStringDescription,
        "moduleHtml": getModuleHtml,
        # "outputPorts": getOutputPortsDescription,
        "parameters": getParametersString
    }
    #spawn the module and wait for its information to arrive at the hub
    mod = spawn(target)
    barrier()
    contentList = readAdditionalDocumentation(filename)
    tagPos = findTagPositions(contentList, tag_functions.keys())
    for tag, pos in tagPos.items():
        line = contentList[pos]
        if tag in tag_functions.keys():
            line = tag_functions[tag](mod)
        elif REG_IMAGE.match(line):
            line = relinkImages(line, sourceDir, destDir)
        contentList[pos] = line

    with open(destDir + target + ".md", "w") as f:
        unusedTags = tag_functions.keys() - tagPos.keys()
        if "headline" in unusedTags:
            f.write(tag_functions["headline"]( mod ))
        [f.write(line) for line in contentList]
        [f.write(tag_functions[unusedTag](mod)) for unusedTag in sorted(unusedTags) if unusedTag != "headline"]
    quit()

#-------------Main-------------#
if __name__ == "__main__":
    # use main for testing your functions
    tag_functions = {
        "headline": getModuleHeadLine,
        "inputPorts": getInputPortsStringDescription,
        "moduleHtml": getModuleHtml,
        "outputPorts": getOutputPortsDescription,
        "parameters": getParametersString
    }
    source = "/home/mdjur/program/vistle/module/general/BoundingBox/BoundingBox.md"
    sourceDir = "/home/mdjur/program/vistle/module/general/BoundingBox"
    destDir = os.path.dirname(os.path.realpath(__file__)) + "/moduleDescriptions/"
    contentList = readAdditionalDocumentation(source)
    searchTags = tag_functions.keys()
    tagPos = findTagPositions(contentList, tag_functions.keys())

    for tag, pos in tagPos.items():
        line = contentList[pos]
        line = relinkImages(line, sourceDir, destDir)
        print(line)

    # for unused in (tag_functions.keys() - tagPos.keys()):
    #     print(unused)
