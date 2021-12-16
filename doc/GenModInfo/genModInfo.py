import time
import os
from bisect import insort
from _vistle import getModuleName, getModuleDescription, getInputPorts, getOutputPorts, getPortDescription, getParameters, getParameterTooltip, getParameterType, spawn, barrier, quit

#-------------Classes--------------#
class Replacement:
    def __init__(self, replacedTag="", replacement="", content=[]):
        self.replacedTag = replacedTag
        self.replacement = replacement
        self._initTagPos(content)

    def _initTagPos(self, content):
        self.tagFoundPos = content.find(self.replacedTag)

    def __lt__(self, other):
        self.tagFoundPos < other.tagFoundPos

class ImageSignature:
    def __init__(self, beginSequenz, endSequenz):
        self.beginSequenz = beginSequenz
        self.endSequenz = endSequenz

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
            content = f.read()
    return content

def relinkImages(file, sourceDir, destDir):
    imageSignatures = [ImageSignature("![](", ")"), ImageSignature("<img src=\"", "\"")]
    positions = []
    for imageSignature in imageSignatures:
        begin = -1
        end = 0
        while True:
            begin = file[end : ].find(imageSignature.beginSequenz)
            if begin == -1:
                break
            begin += end + len(imageSignature.beginSequenz)
            end = begin + file[begin : ].find(imageSignature.endSequenz)
            positions.append((begin, end))
    if len(positions) == 0:
        return file
    positions.sort(key=lambda pos : pos[0])
    newFile = file[0:positions[0][0]]
    for i in range(len(positions)):
        image = file[positions[i][0] : positions[i][1]]
        if image[0] != "/": #keep absolute path (altough they don't seem to work)
            image = os.path.relpath(sourceDir, destDir) + "/" + image
        end = len(file)
        if i < len(positions) - 1:
            end = positions[i+1][0]
        newFile += image + file[positions[i][1] : end]

    return newFile

# <img src="isosurfaceExample.png" alt="drawing" style="width:600px;"/>
def removeEmptyLinesFromEnd(file):
    while True:
        if file[len(file) - 1] == '\n':
            file = file[0 : len(file) - 1]
        else:
            break
    file += "\n"
    return file

#-------------Main-------------#
def generateModuleDescriptions():
    #these have to be set to chose the module to create documentation for
    target = os.environ['VISTLE_DOCUMENTATION_TARGET']
    sourceDir = os.environ['VISTLE_DOCUMENTATION_DIR']
    filename = sourceDir +  "/" + target + ".md"

    #spawn the module and wait for its information to arrive at the hub
    mod = spawn(target)
    barrier()
    content = readAdditionalDocumentation(filename)
    destDir =os.path.dirname(os.path.realpath(__file__)) + "/moduleDescriptions/"
    content = relinkImages(content, sourceDir, destDir)

    #these replacements get inserted at in the additional documentation at the beginning or
    #if the tag is used it replaces this tag
    tag_functions = (
        ("headline", getModuleHeadLine),
        #("inputPorts", getInputPortsStringDescription),
        ("moduleHtml", getModuleHtml),
        #("outputPorts", getOutputPortsDescription),
        ("parameters", getParametersString)
    )
    replacements = []
    for tag, func in tag_functions:
        rpl = Replacement(replacedTag=getComment(tag), replacement=func(mod), content=content)
        insort(replacements, rpl)
    newContent = ""

    #replace the tags with the replacement
    #sort to use the used tags in the wright order
    oldPos = 0
    for replacement in replacements:
        if replacement.tagFoundPos < 0:
            newContent += replacement.replacement
            continue
        newContent += content[oldPos : replacement.tagFoundPos] + replacement.replacement
        oldPos = replacement.tagFoundPos + len(replacement.replacedTag)
    newContent += content[oldPos :]
    newContent = removeEmptyLinesFromEnd(newContent)
    with open(destDir + target + ".md", "w") as f:
        f.write(newContent)
    quit()
