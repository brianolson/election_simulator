#!/usr/bin/python

import csv
import getopt
import os
import re
import string
import sys
import time
import urllib

int_pattern = re.compile("^[-+]?[0-9]+$")
float_pattern = re.compile(r"^[-+]?[0-9]*\.?[0-9]+[eE]?[0-9]*$")

free_variables = ['Happiness', 'System Std', 'Voter Happiness Std', 'Gini']

default_format = 'l${sets}_x${x}/${combos}_${y}.svg'
default_template = string.Template(default_format)

index_path_template = string.Template('l${sets}_x${x}__${combos}.html')

index_head = string.Template(
    '<html><head><title>l${sets}_x${x}/${combos}</title></head><body>'
    '<p>Various ${sets} over ${x} for:')

def comboString(combo_names, combo_values):
	combo_tuples = zip(combo_names, combo_values)
	return "_".join(["%s=%s" % x for x in combo_tuples])

def buildPath(xname, yname, setsname, combo_names, combo_values, tpl=default_template):
	combos = comboString(combo_names, combo_values)
	path = tpl.safe_substitute(
		{'x': xname, 'y': yname, 'sets': setsname, 'combos': combos})
	return path

def buildIndexPath(xname, setsname, combo_names, combo_values, tpl=index_path_template):
	combos = comboString(combo_names, combo_values)
	path = tpl.safe_substitute(
		{'x': xname, 'sets': setsname, 'combos': combos})
	return path

def indexPageTop(xname, setsname, combo_names, combo_values, tpl=index_path_template):
	combos = comboString(combo_names, combo_values)
	out = index_head.safe_substitute({
	    'x': xname, 'sets': setsname, 'combos': combos})
	for name, value in zip(combo_names, combo_values):
		out += '<br />\n%s = %s' % (name, value)
	out += '</p>\n<table>'
	return out

def imageCell(svgpath):
	return '<td><a href="' + urllib.quote(svgpath) + '"><img src="' + urllib.quote(svgpath.replace('svg','png')) + '"></a></td>'

def openWithMakedirs(path, mode):
	(dir, fname) = os.path.split(path)
	if dir and not os.path.isdir(dir):
		os.makedirs(dir)
	return open(path, mode)

class column(object):
	def __init__(self, name=None):
		self.is_int = True
		self.is_float = True
		self.name = name
		self.data = []
		self.numvals = None
	
	def append(self, x):
		if x is None:
			return
		# don't care what type it is yet
		self.data.append(x)
		if self.is_int and not int_pattern.match(x):
			self.is_int = False
		if self.is_float and not float_pattern.match(x):
			self.is_float = False
	
	def cleanup(self):
		if self.is_int:
			self.data = map(int, self.data)
			return
		if self.is_float:
			self.data = map(float, self.data)
			return

	def getUniqueValues(self):
		h = {}
		for x in self.data:
			h[x] = 1
		out = h.keys()
		out.sort()
		self.numvals = len(out)
		return out
		
	def getNumValues(self):
		if self.numvals is None:
			self.getUniqueValues()
		return self.numvals


def plot(out, xcol, ycol, setcol):
	sets = {}
	for s,x,y in zip(setcol, xcol, ycol):
		sets[x] = (x,y)
	for sk, sd in sets.iteritems():
		out.write("%s\n" % sk)
		for x,y in sd:
			out.write("%s\t%s\n" % (x, y))
		out.write("\n")
	out.flush()

def getPlotSets(xname, yname, setsname, colhash,
	  filterargs=None, filterfunc=None):
	"""Plot based on data in hash of columns.

	Args:
	  xname: name of column in colhash for x axis
	  yname: name of column in colhash for y axis
	  setsname: name of column in colhash for data series
	  filterargs: list of names in colhash for arguments to filterfunc
	  filterfunc([]): given data from columns (in a list), return true if data should be plotted.

	Return: hash based on sets. {name: [list of (x,y) tuples]}
	"""
	sets = {}
	xcol = colhash[xname]
	if xcol is None:
		raise Error("invalid column name \"%s\"" % xname)
	ycol = colhash[yname]
	if ycol is None:
		raise Error("invalid column name \"%s\"" % yname)
	scol = colhash[setsname]
	if scol is None:
		raise Error("invalid column name \"%s\"" % setsname)
	for i in xrange(0, len(xcol.data)):
		if filterargs and filterfunc:
			arglist = [colhash[x].data[i] for x in filterargs]
			if not filterfunc(arglist):
				continue
		setkey = scol.data[i]
		setlist = None
		if setkey not in sets:
			setlist = []
			sets[setkey] = setlist
		else:
			setlist = sets[setkey]
		setlist.append( (xcol.data[i], ycol.data[i]) )
	return sets

def gnuplotSets(out, sets):
	setnames = sets.keys()
	print "plotting sets: " + ", ".join(setnames)
	def settitle(sn):
		return "'-' title \"%s\"" % sn
	titlepart = ", ".join(map(settitle, setnames))
	out.write("plot %s\n" % titlepart)
	for sn in setnames:
		setdata = sets[sn]
		for x,y in setdata:
			out.write("%s\t%s\n" % (x,y))
		out.write("e\n")
	out.write("\n")

def nmax(*a):
	"""My max function always prefers a value over None."""
	max = None
	for i in a:
		if (max is None) or ((i is not None) and (i > max)):
			max = i
	return max

def nmin(*a):
	"""My min function always prefers a value over None."""
	min = None
	for i in a:
		if (min is None) or ((i is not None) and (i < min)):
			min = i
	return min

svg_prologue = """<?xml version="1.0" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg version="1.1" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1600 1000">
"""

svg_colors = [
	'#00ff00', '#0000ff',
	'#800080', '#00ffff', '#ff0000',
	'#008000', '#ffa500', '#808000',
	'#000080', '#808080', '#800000',
	'#000000', '#ff00ff', '#ffff00',
	]

def svgSets(out, sets, xlabel=None, ylabel=None):
	setnames = sets.keys()
	if not setnames:
		return
	ytop = 50
	ybottom = 900
	xleft = 100
	xright = 1300
	text_height = 21
	minx = None
	maxx = None
	miny = None
	maxy = None
	avhighs = {}
	for sn in setnames:
		sumy = 0.0
		county = 0
		for x,y in sets[sn]:
			minx = nmin(minx, x)
			maxx = nmax(maxx, x)
			miny = nmin(miny, y)
			maxy = nmax(maxy, y)
			sumy += y
			county += 1
		avhighs[sn] = sumy / county
	setnames.sort(lambda a,b: cmp(avhighs[a], avhighs[b]), reverse=True)
	#print "plotting sets: " + ", ".join(map(str, setnames))
	xmult = (xright - xleft) / (maxx - minx)
	ymult = (ytop - ybottom) / (maxy - miny)
	gx = None
	gy = None
	out.write("""<g font-size="%d">\n""" % text_height)
	out.write("""<text x="%d" y="%d" alignment-baseline="middle" text-anchor="end">%s</text>""" %
		(xleft - 5, ytop, maxy))
	out.write("""<path fill="none" stroke-width="1" stroke="black" d="M %d %d L %d %d" />\n""" %
		(xleft - 4,ytop, xleft,ytop))
	out.write("""<text x="%d" y="%d" alignment-baseline="middle" text-anchor="end">%s</text>""" %
		(xleft - 5, ybottom, miny))
	out.write("""<path fill="none" stroke-width="1" stroke="black" d="M %d %d L %d %d" />\n""" %
		(xleft - 4,ybottom, xleft,ybottom))
	xvals = {}
	colori = -1
	for sn in setnames:
		setdata = sets[sn]
		colori = (colori + 1) % len(svg_colors)
		out.write("""<path fill="none" stroke-width="1" stroke="%s" """
			% svg_colors[colori])
		out.write("d=\"M")
		first = True
		for x,y in setdata:
			gx = ((x - minx) * xmult) + xleft
			gy = ((y - miny) * ymult) + ybottom
			out.write(" %d %d" % (gx,gy))
			if first:
				first = False
				out.write(" L")
			xvals[x] = 1
		out.write("\" />\n")
		if False:
			out.write("""<text x="%d" y="%d" alignment-baseline="middle" stroke="%s">%s</text>\n""" %
				(gx, gy, svg_colors[colori], sn))
	colori = -1
        yNameColors = []
	for sn in setnames:
		setdata = sets[sn]
                _, lasty = setdata[-1]
                lasty = gy = ((lasty - miny) * ymult) + ybottom
		colori = (colori + 1) % len(svg_colors)
                yNameColors.append( (lasty, sn, colori) )

        yNameColors.sort()
        print yNameColors
	lx = xright
	ly = ytop
        for lasty, sn, colori in yNameColors:
                if lasty > ly:
                        ly = lasty
		out.write("""<text x="%d" y="%d" alignment-baseline="middle" fill="%s">%s</text>\n""" %
			(lx, ly, svg_colors[colori], sn))
                # set minimum next label y
		ly += text_height * (9/7)
	lastgx = None
	xvallist = xvals.keys();
	xvallist.sort()
	for x in xvallist:
		gx = ((x - minx) * xmult) + xleft
		out.write("""<path fill="none" stroke-width="1" stroke="black" d="M %d %d L %d %d" />\n""" %
			(gx, ybottom, gx, ybottom - 5))
		if (lastgx is not None) and ((gx - lastgx) < (text_height * 2)):
			continue
		out.write("""<text x="%d" y="%d" alignment-baseline="text-before-edge" text-anchor="middle">%s</text>\n""" %
			(gx, ybottom, x))
		lastgx = gx
	if xlabel is not None:
		out.write("""<text x="%d" y="%d" alignment-baseline="text-before-edge" text-anchor="middle" font-size="180%%">%s</text>\n""" %
			(((xright + xleft) / 2), ybottom + text_height, xlabel))
	if ylabel is not None:
		out.write("""<text alignment-baseline="text-before-edge" text-anchor="middle" font-size="180%%" transform="translate(%d, %d) rotate(90)">%s</text>\n""" %
			(xleft - text_height, ((ybottom + ytop) / 2), ylabel))
	out.write("""</g>\n""")

noshowMethods = [
	"Borda, truncated",
	"Maximized Rating Summation",
	"Rating Summation, 1..num choices",
	"Rating Summation, 1..10",
	"Instant Runoff Normalized Ratings, positive shifted",
	"Iterated Normalized Ratings",
	]

def eqbut(eq):
	def tmfu(x):
		for i in xrange(0,len(eq)):
			if x[i] != eq[i]:
				return False
		if x[-1] in noshowMethods:
			return False
		return True
	return tmfu

def product(l):
	return reduce(lambda x,y: x * y, l)

def permute_choices(l, choose):
	"""Generate lists of $choose elements from $l."""
	indecies = range(0,choose)
	while indecies[0] < len(l) - choose + 1:
		yield [l[i] for i in indecies]
		pos = choose - 1
		while pos < choose:
			indecies[pos] += 1
			# if indecies[choose - 1] >= len(l)
			# if indecies[choose - 2] >= len(l) - 1
			if indecies[pos] >= len(l) - ((choose - 1) - pos):
				pos -= 1
				if pos < 0:
					raise StopIteration
			else:
				pos += 1
				while pos < choose:
					indecies[pos] = indecies[pos - 1] + 1
					pos += 1

def permute_sets(*sets):
	"""Generate lists of combinations of elements from lists in sets."""
	iterators = [x.__iter__() for x in sets]
	if len(iterators) == 0:
		raise StopIteration
	values = [x.next() for x in iterators]
	if len(values) == 0:
		raise StopIteration
	while True:
		yield list(values)
		pos = 0
		goForward = True
		while goForward:
			try:
				values[pos] = iterators[pos].next()
				goForward = False
			except StopIteration:
				pos += 1
			if pos >= len(iterators):
				raise StopIteration
		while pos > 0:
			pos -= 1
			iterators[pos] = sets[pos].__iter__()
			values[pos] = iterators[pos].next()

def stepSteppable(steppable_columns, f):
	"""Run f(xaxis, setaxis, [others]) over columns list steppable_columns"""
	for xaxis in steppable_columns:
		if not xaxis.is_float:
			continue
		notx = list(steppable_columns)
		notx.remove(xaxis)
		for setaxis in notx:
			graphsets = list(notx)
			graphsets.remove(setaxis)
			f(xaxis, setaxis, graphsets)

def generateSteppable(steppable_columns):
	"""yield (xaxis, setaxis, [others]) over columns list steppable_columns"""
	for xaxis in steppable_columns:
		if not xaxis.is_float:
			continue
		notx = list(steppable_columns)
		notx.remove(xaxis)
		for setaxis in notx:
			graphsets = list(notx)
			graphsets.remove(setaxis)
			yield (xaxis, setaxis, graphsets)


def filename_sanitize(x):
	return x.replace(" ", "_").replace(",", "_")

def myStrNumEq(a, b):
	try:
		fa = float(a)
		fb = float(b)
		return fa == fb
	except:
		pass
	return a == b

def csvToColumnList(csvFile, restricts=None):
	"""csvFile iterator that returns lists of table cells (a csv reader). first row is headers.
	restricts list of [colname, value, positive] to filter input on
	returns list of column objects full of data
	"""
	cr = csv.reader(csvFile)
	headerRow = cr.next()
	indexValueFilters = []
	columnlist = map(column, headerRow)
	if restricts:
		for i in xrange(0, len(columnlist)):
			for pair in restricts:
				if pair[0] == columnlist[i].name:
					indexValueFilters.append( (i, pair[1], pair[2]) )
	rowcount = 0
	for row in cr:
		doRow = True
		for i,v,p in indexValueFilters:
			test = myStrNumEq(row[i], v)
			if (p and not test) or ((not p) and test):
				doRow = False
				break
		if doRow:
			def ca(col, x):
				if col is None:
					return
				col.append(x)
			map(ca, columnlist, row)
	for c in columnlist:
		c.cleanup()
	return columnlist


def truefalse(x):
	"""Return True/False based on string input.
	There really ought to be a standard function for this."""
	if x is None:
		return False
	if x.lower() == 'true':
		return True
	if x.lower() == 'false':
		return False
	try:
		return bool(int(x))
	except:
		# not an int? don't care
		pass
	# only false on empty string
	return bool(x)


def matchOutput(oot, combo_names, comboi, pagename, pagevalue):
	ootparams = oot[1]
	if pagename not in ootparams:
		return False
	if ootparams[pagename] != pagevalue:
		return False
	for n,v in zip(combo_names, comboi):
		if n not in ootparams:
			return False
		if ootparams[n] != v:
			return False
	return True


usage = """./plotrlog.py foo.csv [options]
 -x a/--x=a  limit x axis to 'a'
 --set=a     only run set axis 'a'
 --only=a=b  only accept data where column 'a' has value 'b'
"""

def main(argv):
	# "rlog.csv"
	finame = None
	xaxis_opt = None
	setaxis_opt = None
	do_gnuplot = False
	do_svgplot = False
	restricts = []
	optlist, args = getopt.gnu_getopt(argv[1:], "i:x:", ['in=', 'only=', 'x=', 'set=', 'sets=', 'svg=', 'dosvg', 'not='])
	for option, value in optlist:
		if option == '-i' or option == '--in':
			if finame is None:
				finame = value
			else:
				sys.stderr.write("multiple inputs specified but only one allowed\n")
				sys.exit(1)
		elif option == '-x' or option == '--x':
			xaxis_opt = value
			print "limiting to x axis \"%s\"" % xaxis_opt
		elif option == '--set' or option == '--sets':
			setaxis_opt = value
		elif option == '--only':
			ra = value.split("=")
			ra.append(True)
			restricts.append(ra)
		elif option == '--not':
			ra = value.split("=")
			ra.append(False)
			restricts.append(ra)
		elif option == '--dosvg':
			do_svgplot = True
		elif option == '--svg':
			do_svgplot = truefalse(value)
		else:
			sys.stderr.write("bogus option=\"%s\" value=\"%s\"\n" % (option, value))
			sys.exit(1)
	if finame is None:
		if len(args) == 1:
			finame = args[0]
		elif len(args) > 1:
			sys.stderr.write("multiple inputs specified but only one allowed\n")
			sys.exit(1)
		else:
			sys.stderr.write("no input specified. use -i/--in\n")
			sys.exit(1)
	
	start_time = time.time()
	columnlist = csvToColumnList(open(finame,"rb"), restricts)
	parse_end_time = time.time()
	dt = parse_end_time - start_time
	print "loaded %d rows in %f seconds" % (len(columnlist[0].data), dt)
	
	colhash = {}
	steppable_columns = []
	for x in columnlist:
		colhash[x.name] = x
		if x.name in free_variables:
			print "%s (int=%s, float=%s): free" % (x.name, x.is_int, x.is_float)
		elif x.name == 'Runs':
			pass
		else:
			uvals = x.getUniqueValues()
			if len(uvals) > 30:
				uvalstr = "(%d values)" % len(uvals)
			else:
				uvalstr = ", ".join(map(str, uvals))
				if len(uvals) > 1:
					steppable_columns.append(x)
			print "%s (int=%s, float=%s): %s" % (x.name, x.is_int, x.is_float, uvalstr)
	print ""
	print "steppable columns: " + ", ".join(map(lambda x: x.name, steppable_columns))
	
	graph_combo_count = 0
	for (xaxis, setaxis, graphsets) in generateSteppable(steppable_columns):
		if (xaxis_opt is not None) and (xaxis.name not in xaxis_opt):
			continue
		if (setaxis_opt is not None) and (setaxis_opt != setaxis.name):
			continue
		combo_names = [x.name for x in graphsets]
		#graph_combos = product(map(lambda x: x.getNumValues(), graphsets))
		#graph_combo_count += graph_combos
		#print "x axis = %s, sets = %s: %d graphs over {%s}" % (
		#	xaxis.name, setaxis.name, graph_combos,
		#	", ".join(map(lambda x: x.name, graphsets))
		#	)
		outputs = []
		for comboi in permute_sets(*(l.getUniqueValues() for l in graphsets)):
			for y in free_variables:
				outpath = buildPath(xaxis.name, y, setaxis.name, combo_names, comboi)
				outputs.append( (outpath, dict(zip(combo_names, comboi)), y) )
				print outpath
				graph_combo_count += 1
				sets = getPlotSets(
					xaxis.name, y, setaxis.name, colhash,
					combo_names, eqbut(comboi))
				if do_svgplot:
					outfile = openWithMakedirs(outpath, "w")
					outfile.write(svg_prologue)
					svgSets(outfile, sets, xaxis.name, y)
					outfile.write("</svg>\n")
					outfile.close()
		for pageset in graphsets:
			othersets = list(graphsets)
			othersets.remove(pageset)
			combo_names = [x.name for x in othersets]
			for comboi in permute_sets(*(l.getUniqueValues() for l in othersets)):
				outpath = buildIndexPath(xaxis.name, setaxis.name, combo_names, comboi)
				indexfile = openWithMakedirs(outpath, "w")
				indexfile.write(indexPageTop(xaxis.name, setaxis.name, combo_names, comboi))
				for pagevalue in pageset.getUniqueValues():
					indexfile.write('<tr><td>%s = %s</td>' % (pageset.name, pagevalue))
					for oot in outputs:
						if matchOutput(oot, combo_names, comboi, pageset.name, pagevalue):
							indexfile.write(imageCell(oot[0]))
					indexfile.write('</tr>\n')
				indexfile.write('</table></body></html>\n')
				indexfile.close()
							
					
		#TODO vary one of graphsets/free_variables and make HTML page
	print "total graphs: %d" % graph_combo_count

if __name__ == "__main__":
	main(sys.argv)
