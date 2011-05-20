'''
Copyright (C) 2011 Felipe Correa da Silva Sanches

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
'''

import inkex
import sys

logfile = open("/tmp/inkscape-ext-logfile", "wa")
def LOG(s):
	logfile.write(s+"\n")

class Layers2SVGFont(inkex.Effect):
	def __init__(self):
		inkex.Effect.__init__(self)

	def guideline_value(self, label, index):
		namedview = self.svg.find(inkex.addNS('namedview', 'sodipodi'))
		guides = namedview.findall(inkex.addNS('guide', 'sodipodi'))
		for guide in guides:
			l=guide.get(inkex.addNS('label', 'inkscape'))
			if l==label:
				return int(guide.get("position").split(",")[index])
		return 0

	def get_or_create(self, parentnode, nodetype):
		node = parentnode.find(nodetype)
		if node is None:
			node = inkex.etree.SubElement(parentnode, nodetype)
		return node

	def get_or_create_glyph(self, font, unicode_char):
		glyphs = font.findall(inkex.addNS('glyph', 'svg'))
		for glyph in glyphs:
			if unicode_char == glyph.get("unicode"):
				return glyph
		return inkex.etree.SubElement(font, inkex.addNS('glyph', 'svg'))	

	def effect(self):
		# Get access to main SVG document element
		self.svg = self.document.getroot()
		self.defs = self.get_or_create(self.svg, inkex.addNS('defs', 'svg'))

		setwidth = int(self.svg.get("width"))
		baseline = self.guideline_value("baseline", 1)
		ascender = self.guideline_value("ascender", 1) - baseline
		caps = self.guideline_value("caps", 1) - baseline
		xheight = self.guideline_value("xheight", 1) - baseline
		descender = self.guideline_value("descender", 1) - baseline
		lbearing = self.guideline_value("lbearing", 0)
		rbearing = setwidth - self.guideline_value("rbearing", 0)

		LOG("setwidth: " + str(setwidth))
		LOG("baseline: " + str(baseline))
		LOG("ascender: " + str(ascender))
		LOG("descender: " + str(descender))
		LOG("caps: " + str(caps))
		LOG("xheight: " + str(xheight))
		LOG("lbearing: " + str(lbearing))
		LOG("rbearing: " + str(rbearing))

		font = self.get_or_create(self.defs, inkex.addNS('font', 'svg'))
		font.set("horiz-adv-x", str(setwidth))

		fontface = self.get_or_create(font, inkex.addNS('fontface', 'svg'))
		fontface.set("units-per-em", str(setwidth))
		fontface.set("cap-height", str(caps))
		fontface.set("x-height", str(xheight))
		fontface.set("ascent", str(ascender))
		fontface.set("descent", str(descender))
		#TODO: should we somehow store sidebearing values?

		groups = self.svg.findall(inkex.addNS('g', 'svg'))
		for group in groups:
			label = group.get(inkex.addNS('label', 'inkscape'))
			if "GlyphLayer-" in label:
				unicode_char = label.split("GlyphLayer-")[1]
				glyph = self.get_or_create_glyph(font, unicode_char)
				glyph.set("unicode", unicode_char)
				use = self.get_or_create(glyph, inkex.addNS('use', 'svg'))
				use.set(inkex.addNS('href', 'xlink'), "#"+group.get("id"))
				#TODO: This code creates nodes but they do not render on svg fonts dialog. why? 

if __name__ == '__main__':
	LOG("\n\n"+"*"*80+"\n\n")
	e = Layers2SVGFont()
	e.affect()
	logfile.close()

