#! /usr/bin/env python

import sys
import re

# titlecase for a single word
def titleword(w):
    article = ["a", "an", "the"]
    conjunction = ["and", "but", "for", "or", "nor", "as"]
    preposition = ["to", "at", "by", "of", "on", "from", "in", "into"]
    other = []
    small = article + conjunction + preposition + other

    if w in small:
        return w
    return w.capitalize()

# titlecase for a group of words
def titlecase(s):
    return re.sub(
        r"[-A-Za-z]+('[-A-Za-z]+)?",
        lambda word: titleword(word.group(0)),
        s.capitalize())

n = len(sys.argv)
for i in range(1, n):
    print(titlecase(sys.argv[i]), end = " ")
