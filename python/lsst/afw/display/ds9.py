#
# XPA
#
import os, re, math, sys, time

try: import xpa
except: print "Cannot import xpa"

import displayLib
import lsst.afw.image as afwImage

class Ds9Error(IOError):
    """Some problem talking to ds9"""

#
# Symbolic names for mask colours
#
WHITE = "white"; BLACK = "black"
RED = "red"; GREEN = "green"; BLUE = "blue"
CYAN = "cyan"; MAGENTA = "magenta"; YELLOW = "yellow"
_maskColors = [WHITE, BLACK, RED, GREEN, BLUE, CYAN, MAGENTA, YELLOW]

def setMaskPlaneColor(name, color=None):
    """Request that mask plane name be displayed as color; name may be a dictionary
    (in which case color should be omitted"""

    if isinstance(name, dict):
        assert color == None
        for k in name.keys():
            setMaskPlaneColor(k, name[k])
        return

    global _maskPlaneColors
    try:
        type(_maskPlaneColors)
    except:
        _maskPlaneColors = {}

    for c in _maskColors:
        if color == c:
            _maskPlaneColors[name] = color
            return

    raise RuntimeError, "%s is not a supported colour" % color
#
# Default mapping from mask plane names to colours
#
setMaskPlaneColor({
    "BAD": RED,
    "CR" : MAGENTA,
    "EDGE": YELLOW,
    "INTRP" : GREEN,
    "SAT" : GREEN,
    "DETECTED" : BLUE,
    })

def getMaskPlaneColor(name):
    """Return the colour associated with the specified mask plane name"""

    if _maskPlaneColors.has_key(name):
        return _maskPlaneColors[name]
    else:
        return None

def setMaskPlaneVisibility(name, show=True):
    """Specify the visibility of a given mask plane; name may be a dictionary (in which case show will be ignored)"""

    global _maskPlaneVisibility
    try:
        type(_maskPlaneVisibility)
    except NameError, e:
        _maskPlaneVisibility = {}

    if isinstance(name, dict):
        for k in name.keys():
            setMaskPlaneVisibility(k, name[k])
        return

    _maskPlaneVisibility[name] = show

setMaskPlaneVisibility({})

def getMaskPlaneVisibility(name):
    """Should we display the specified mask plane name?"""

    if _maskPlaneVisibility.has_key(name):
        return _maskPlaneVisibility[name]
    else:
        return True

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def ds9Cmd(cmd):
   """Issue a ds9 command, raising errors as appropriate"""
   
   try:
      xpa.set(None, "ds9", cmd, "", "", 0)
   except IOError:
      raise Ds9Error, "XPA: (%s)" % cmd

def initDS9(execDs9 = True):
   try:
      ds9Cmd("iconify no; raise")
      ds9Cmd("wcs wcsa")                # include the pixel coordinates WCS (WCSA)
   except IOError:
      if execDs9:
         print "ds9 doesn't appear to be running, I'll exec it for you"
         if not re.search('xpa', os.environ['PATH']):
            raise Ds9Error, 'You need the xpa binaries in your path to use ds9 with python'

         os.system('ds9 &')
         for i in range(0,10):
            try:
               ds9Cmd("frame 0; scale histequ; scale mode minmax")
               break
            except IOError:
               print "waiting for ds9...\r",
               time.sleep(0.5)
            else:
               break

         print "                  \r",
         sys.stdout.flush();

      raise Ds9Error

def setMaskColor(color = GREEN):
    """Set the ds9 mask colour to; eg. ds9.setMaskColor(ds9.RED)"""
    ds9Cmd("mask color %s" % color)

def mtv(data, frame=0, init=True, WCS=None, isMask=False):
   """Display an Image or Mask on a DS9 display

Historical note: the name "mtv" comes from Jim Gunn's forth imageprocessing
system, Mirella (named after Mirella Freni); The "m" stands for Mirella.
   """
	
   if frame == None:
      return
   
   if init:
      for i in range(0,3):
         try:
            initDS9(i == 0)
         except IOError:
            print "waiting for ds9...\r", ; sys.stdout.flush();
            time.sleep(0.5)
         else:
            break
         
   ds9Cmd("frame %d" % frame)

   if re.search("::MaskedImage<", data.repr()): # it's a MaskedImage; display the Image and overlay the Mask
       _mtv(data.getImage(), WCS, False)
       mtv(data.getMask(), frame, False, WCS, False)
   elif re.search("::Mask<", data.repr()): # it's a Mask; display it, bitplane by bitplane
       nMaskPlanes = data.getNumPlanesUsed()
       maskPlanes = data.getMaskPlaneDict()
       cols, rows = data.getCols(), data.getRows()

       planes = {}                      # build inverse dictionary
       for key in maskPlanes.keys():
           planes[maskPlanes[key]] = key

       colorIndex = 0                   # index into maskColors
       for p in range(nMaskPlanes):
           if planes[p]:
               if not getMaskPlaneVisibility(planes[p]):
                   continue

               mask = data.getSubMask(afwImage.BBox2i(0, 0, cols, rows)) # the only way to get a copy
               mask &= (1 << p)

               color = getMaskPlaneColor(planes[p])

               if not color:            # none was specified
                   while True:
                       color = _maskColors[colorIndex%len(_maskColors)]; colorIndex += 1
                       if color != WHITE and color != BLACK:
                           break

               setMaskColor(color)
               _mtv(mask, WCS, True)
       return
   elif re.search("::Image<", data.repr()): # it's an Image; display it
       _mtv(data, WCS, False)
   else:
       raise RuntimeError, "Unsupported type %s" % data.repr()

def _mtv(data, WCS=None, isMask=False):
   """Internal routine to display an Image or Mask on a DS9 display"""

   if True:
       if isMask:
           xpa_cmd = "xpaset ds9 fits mask"
       else:
           xpa_cmd = "xpaset ds9 fits"
           
       pfd = os.popen(xpa_cmd, "w")
   else:
      pfd = file("foo.fits", "w")

   try:
       #import pdb; pdb.set_trace()
       try:
           displayLib.writeFitsImage(pfd.fileno(), data, WCS)
       except NotImplementedError:
           displayLib.writeFitsImage(pfd.fileno(), data.get(), WCS)
   except Exception, e:
       try:
           pfd.close()
       except:
           pass
       
       raise e

   try:
       pfd.close()
   except:
       pass
#
# Graphics commands
#
def erase(frame = 0, len = 2):
   """Erase the specified DS9 frame"""
   if frame == None:
      return

   ds9Cmd("frame %d; regions delete all" % frame)

def dot(symb, r, c, frame = 0, size = 2, ctype = 'green'):
   """Draw a symbol onto the specfied DS9 frame at (row,col) = (r,c) [0-based coordinates]
Possible values are:
	+	Draw a +
	x	Draw an x
        o	Draw a circle
Any other value is interpreted as a string to be drawn
"""
   if frame == None:
      return

   cmd = "frame %d; " % frame
   r += 1; c += 1;                      # ds9 uses 1-based coordinates
   if (symb == '+'):
      cmd += 'regions command {line %g %g %g %g}; ' % (c, r+size, c, r-size)
      cmd += 'regions command {line %g %g %g %g}; ' % (c-size, r, c+size, r)
   elif (symb == 'x'):
      size = size/math.sqrt(2)
      cmd += 'regions command {line %g %g %g %g}; ' % (c+size, r+size, c-size, r-size)
      cmd += 'regions command {line %g %g %g %g}; ' % (c-size, r+size, c+size, r-size)
   elif (symb == 'o'):
      cmd += 'regions command {circle %g %g %g}; ' % (c, r, size)
   else:
      cmd += 'regions command {text %g %g \"%s\"}; ' % (c, r, symb)

   #cmd += ' # color=%s' % ctype

   ds9Cmd(cmd)

def line(points, frame = 0, symbs = False, ctype = 'green'):
   """Draw a set of symbols or connect the points, a list of (row,col)
If symbs is True, draw points at the specified points using the desired symbol,
otherwise connect the dots.  Ctype is the name of a colour (e.g. 'red')"""
   
   if frame == None:
      return

   if symbs:
      for (r, c) in points:
         dot(symbs, r, c, frame = frame, size = 0.5, ctype = ctype)
   else:
      if len(points) > 0:
          cmd = "frame %d; " % (frame)

          r0, c0 = points[0];
          r0 += 1; c0 += 1;             # ds9 uses 1-based coordinates
          for (r, c) in points[1:]:
             r += 1; c += 1;            # ds9 uses 1-based coordinates
             cmd += 'regions command { line %g %g %g %g };' % (c0, r0, c, r)
             c0, r0 = c, r
          #cmd += ' # color=%s' % ctype

          ds9Cmd(cmd)
#
# Zoom and Pan
#
def zoom(zoomfac = None, rowc = None, colc = None, frame = 0):
   """Zoom frame by specified amount, optionally panning also"""

   if frame == None:
      return

   if (rowc and not colc) or (not rowc and colc):
      raise Ds9Error, "Please specify row and column center to pan about"
   
   if zoomfac == None and rowc == None:
      zoomfac = 2

   cmd = ""
   if zoomfac != None:
      cmd += "zoom to %d; " % zoomfac

   if rowc != None:
      cmd += "pan to %g %g physical; " % (colc + 1, rowc + 1) # ds9 is 1-indexed. Grrr

   ds9Cmd(cmd)

def pan(rowc = None, colc = None, frame = 0):
   """Pan to (rowc, colc); see also zoom"""
   zoom(None, rowc, colc, frame)
