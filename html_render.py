#!/usr/bin/env python3

"""
A class-based system for rendering html.
"""


# This is the framework for the base class
class Element:
    
    tag = "html"

    def __init__(self, content=None, **kwargs):
        self.attributes = kwargs
        if content is None:
            self.content = []
        else:
            self.content = [content]


    def append(self, new_content):
        self.content.append(new_content)


    def render(self, out_file):
        
        open_tag = ["<{}".format(self.tag)]
        for key, value in self.attributes.items():
            open_tag.append(" "+key+"="+ "\"" + value + "\",")
        if(open_tag[-1][-1] == ','):
            open_tag[-1] = open_tag[-1][:-1]
        open_tag.append(">\n")
        out_file.write("".join(open_tag))
        for line in self.content:
            try:
                line.render(out_file)
            except AttributeError:
                out_file.write(line)
            out_file.write("\n")
        out_file.write("</{}>\n".format(self.tag))



class Html(Element):
    tag = "html"
    pass


class Body(Element):
    tag = "body"
    pass


class P(Element):
    tag = "p"
    pass


class Head(Element):
    tag = "head"
    pass


class OneLineTag(Element):    
    def render(self, out_file):
        out_file.write("<{}>".format(self.tag))
        out_file.write(self.content[0])
        out_file.write("</{}>\n".format(self.tag))

    def append(self, new_content):
        raise NotImplementedError


class Title(OneLineTag):
    tag = "title"


class SelfClosingTag(Element):

    def append(self, *args):
        raise TypeError("You can not add content to a SelfClosingTag")


    def __init__(self, content=None, **kwargs):
        if content is not None:
            raise TypeError("SelfClosingTag can not contain any content")
        super().__init__(content=content, **kwargs)
    pass


class Hr(SelfClosingTag):
    tag = "hr"

    # def render(self, out_file):
    #     out_file.write(self._open_tag())
    #     out_file.write("\n")
    #     for c in self.ccontent:
    #         try

class Br(SelfClosingTag):
    tag = "br"