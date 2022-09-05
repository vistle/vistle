import time
import os
import re
import subprocess
from bisect import insort
import vistle

REG_IMAGE = re.compile(r"(?:\w+(?:-\w+)+|\w*)\.(?:jpg|gif|png|bmp)")
#REG_TAGS_TMPL = r"\[{tag}\]:<(\w*)>"
REG_TAGS_TMPL = r"(?<=\[{tag}\]:<)(.*)(?=>)"

VSL_TAG = "vslFile"

#-------------Functions-------------#
def createModuleImage(name, inPorts, outPorts):
    size = 1.
    letterWidth = 58 / 100 * size
    widths = [len(name) * letterWidth + size, size * (len(inPorts) + 2), size * (len(outPorts) + 2) ]
    width = max(widths)
    height = 3*size
    spiderLegOffsetTop = size * len(inPorts) + size*0.8
    spiderLegOffsetBot = size * len(outPorts) + size*0.8
    image = "<svg width=\"" + str(10 * width) + "em\" height=\"" + str(height + spiderLegOffsetTop + spiderLegOffsetBot) + "em\" >\n"
    image += getStyle(size) + "\n"
    image += getRect(0, spiderLegOffsetTop, width, height, "#64c8c8ff", 0.1, 0.1) + "\n"

    portDistance = size/5
    portStartPos = portDistance
    num = len(inPorts)
    for port, tooltip in inPorts:
        image += getRect(portStartPos, spiderLegOffsetTop, size , size, color="#c81e1eff", tooltip=port) + "\n"
        text = tooltip + "<tspan> ("+ port + ")</tspan>"
        image += getPortDescriptionSpider(size, portStartPos + size/2, spiderLegOffsetTop, num, text) + "\n"
        portStartPos += portDistance + size
        num -= 1

    image += getText(name, portDistance, spiderLegOffsetTop + 1.85*size, "moduleName")

    num = -1 * len(outPorts)
    portStartPos = portDistance
    for port, tooltip in outPorts:
        image += getRect(portStartPos, spiderLegOffsetTop + height - size, size, size, color="#c8c81eff", tooltip=port) + "\n"
        text = tooltip + "<tspan> ("+ port + ")</tspan>"
        image += getPortDescriptionSpider(size, portStartPos + size/2, spiderLegOffsetTop + height, num, text) + "\n"
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
    spider += getRect(x, y - f2* height, size, width, "#000000") + "\n"
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


def getRect(x, y , width, height, color, rx = 0, ry = 0, tooltip = ""):
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
    name = vistle.getModuleName(mod)
    desc = vistle.getModuleDescription(mod)
    return "\n" + getHeadline(name, "#")  + desc + "\n"


def getModuleHtml(mod):
    inPorts = []
    for p in vistle.getInputPorts(mod):
        inPorts.append([p, vistle.getPortDescription(mod, p)])
    outPorts = []
    for p in vistle.getOutputPorts(mod):
        outPorts.append([p, vistle.getPortDescription(mod, p)])
    return  "\n" + createModuleImage(vistle.getModuleName(mod), inPorts, outPorts) + "\n"


def getPortDescriptionString(mod, ports, headline):
    portsDescription = "\n"
    if len(ports) > 0:
        portsDescription += getHeadline(headline, "##")
        portsDescription += getTableLine(["name", "description"]) + getTableLine(["-", "-"])
    for port in ports:
        portsDescription += getTableLine([port,vistle.getPortDescription(mod, port)])
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
        paramString += getHeadline("Parameters", "##") + getTableLine(["name", "description", "type"]) + getTableLine(["-", "-", "-"])
    for param in params:
        paramString += getTableLine([param, vistle.getParameterTooltip(mod, param), vistle.getParameterType(mod, param)])
    return paramString

def getExampleString(mod, example):
    print("getExampleString")
    buildDir = os.environ['VISTLE_DOCLUMENTATION_BIN_DIR']
    sourceDir = os.environ['VISTLE_DOCUMENTATION_DIR']
    destDir = os.path.dirname(os.path.realpath(__file__)) + "/moduleDescriptions/"
    
    imageDir = os.path.relpath(buildDir, destDir) + "/"
    line = "<figure float=\"left\">\n"
    line += "   <img src=\"" + imageDir + example + "_workflow.png\" width=\"" + "200" + "\" />\n" 
    line += "   <img src=\"" + imageDir + example + "_result.png\" width=\"" + "300" + "\" />\n" 
    line += "   <figcaption>Fig.1 " +  example  + " workflow (left) and expected result (right). </figcaption>\n"
    line += "</figure>"

    return line

def genVistleScreenshot(mod, line):
    sourceDir = os.environ['VISTLE_DOCUMENTATION_DIR']
    lineStartWithTag = re.search(r"^" + REG_TAGS_TMPL.format(tag=VSL_TAG), line)
    vslNameWithoutExt = re.search('<(.+?)>', line)
    returnString = ""
    if vslNameWithoutExt:
        modulename = vistle.getModuleName(mod)
        vslName = vslNameWithoutExt.group(1)
        vslDir = sourceDir + "/" + vslName
        imgPath = vslDir + '.png'
        vslPath = vslDir + '.vsl'
        if os.path.exists(vslPath):
            if lineStartWithTag:
                returnString += "![](" + vslName +".png) \n"
            else:
                returnString += re.sub(REG_TAGS_TMPL.format(tag=VSL_TAG), vslName +".png", line)
            subprocess.run(["vistle","--snapshot",imgPath,vslPath])
    return returnString

def readAdditionalDocumentation(filename):
    content = ""
    if os.path.isfile(filename):
        with open(filename, "r") as f:
            content = f.readlines()
    return content


def relinkImage(lineStr, sourceDir, destDir):
    match = re.search(REG_IMAGE, lineStr)
    if match:
        relPath = os.path.relpath(sourceDir, destDir) + "/" + match.group()
        return re.sub(REG_IMAGE, relPath, lineStr)
    return lineStr


def findTagPositions(lines, tags) -> {}:
    idx = 0
    tagPos = {}
    for line in lines:
        tag = isTag(line, tags)
        if tag is None:
            tag = isImage(line)
        if tag != None:
            if (tag in tagPos.keys()):
                tagPos[tag].append(idx)
            else:
                tagPos[tag] = [idx]
        idx += 1
    return tagPos


def isImage(line):
    match = re.search(REG_IMAGE, line)
    if match:
        return match.group()
    return None


def isTag(line, tags): #returns the tag and its value
    for tag in tags:
        matches = re.search(REG_TAGS_TMPL.format(tag=tag), line)
        if matches:
            return [tag, matches.group(1)]
    return [None, None]

    
def generateModuleDescriptions():
    #these have to be set to chose the module to create documentation for
    target = os.environ['VISTLE_DOCUMENTATION_TARGET']
    destDir = os.path.dirname(os.path.realpath(__file__)) + "/moduleDescriptions/"
    sourceDir = os.environ['VISTLE_DOCUMENTATION_DIR']
    #the directory containing the readTheDocs module files
    # ImgDestDir = os.path.dirname(os.path.realpath(__file__)) + "/../../docs/build/html/modules/"

    filename = sourceDir +  "/" + target + ".md"
    
    # tags with the format [tag]:<value> can be replace by asignig a replacement function to the tag:
    essential_tags = { #these can only apear once and if not found these will be added at the end 
        "headline": getModuleHeadLine,
        "moduleHtml": getModuleHtml,
        "parameters": getParametersString
    }

    optional_tags = {
        VSL_TAG: genVistleScreenshot,
        "example": getExampleString
    }

    #spawn the module and wait for its information to arrive at the hub
    mod = vistle.spawn(target)
    vistle.barrier()
    contentList = readAdditionalDocumentation(filename)
    newContentList = []
    for line in contentList:
        tag = isTag(line, essential_tags.keys())
        if tag[0] != None:
            line = essential_tags[tag[0]](mod)
            del essential_tags[tag[0]]
        else:
            tag = isTag(line, optional_tags.keys())
            if tag[0] != None:
                line = optional_tags[tag[0]](mod, tag[1])
        newContentList.append(line)

    #add the missing essential sections
    with open(destDir + target + ".md", "w") as f:
        if "headline" in essential_tags.keys():
            f.write(essential_tags["headline"](mod))
            del essential_tags["headline"]
        [f.write(line) for line in newContentList]
        [f.write(essential_tags[tag](mod)) for tag in essential_tags]
    vistle.quit()

#-------------Main-------------#
if __name__ == "__main__":
    # use main for testing your functions
    tag_functions = {
        "headline": getModuleHeadLine,
        "moduleHtml": getModuleHtml,
        "parameters": getParametersString,
        VSL_TAG: genVistleScreenshot
    }
    source = "/home/mdjur/program/vistle/module/general/ReadTsunami/ReadTsunami.md"
    sourceDir = "/home/mdjur/program/vistle/module/general/ReadTsunami"
    destDir = os.path.dirname(os.path.realpath(__file__)) + "/moduleDescriptions/"
    contentList = readAdditionalDocumentation(source)
    searchTags = tag_functions.keys()
    tagPos = findTagPositions(contentList, tag_functions.keys())

    for tag, posList in tagPos.items():
        for pos in posList:
            line = contentList[pos]
            # if tag == VSL_TAG:
                # line = getExamples(line)
                # exampleName = re.search('<(.+?)>',line)
            line = relinkImage(line, sourceDir, destDir)
            print(line)

    # for unused in (tag_functions.keys() - tagPos.keys()):
    #     print(unused)
