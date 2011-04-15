# 
# LSST Data Management System
# Copyright 2008, 2009, 2010 LSST Corporation.
# 
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the LSST License Statement and 
# the GNU General Public License along with this program.  If not, 
# see <http://www.lsstcorp.org/LegalNotices/>.
#

"""Application Framework image-related classes including Image, Mask and MaskedImage
"""
from imageLib import *
import numpy

suffixes = {str(numpy.uint16): "U", str(numpy.int32): "I", str(numpy.float32): "F", str(numpy.float64): "D"}

def makeImageFromArray(array):
    """Construct an Image from a NumPy array, inferring the Image type from the NumPy type.
    Return None if input is None.
    """
    if array is None: return None
    cls = globals()["Image%s" % suffixes[str(array.dtype.type)]]
    return cls(array)

def makeMaskFromArray(array):
    """Construct an Mask from a NumPy array, inferring the Mask type from the NumPy type.
    """
    if array is None: return None
    cls = globals()["Mask%s" % suffixes[str(array.dtype.type)]]
    return cls(array)

def makeMaskedImageFromArrays(image, mask=None, variance=None):
    """Construct a MaskedImage from three NumPy arrays, inferring the MaskedImage types from the NumPy types.
    """
    cls = globals()["MaskedImage%s" % suffixes[str(image.dtype.type)]]
    return cls(makeImageFromArray(image), makeMaskFromArray(mask), makeImageFromArray(variance))
