#!/usr/bin/env python

import lsst.fw.Core.fwLib as FWCL
import lsst.mwi.data as DATA
import lsst.mwi.persistence as PERS
import lsst.mwi.policy as POL
import os

# Create the additionalData DataProperty
additionalData = DATA.SupportFactory.createPropertyNode("root")
additionalData.addProperty(DATA.DataProperty("sliceId", 0))
additionalData.addProperty(DATA.DataProperty("visitId", "fov391"))
additionalData.addProperty(DATA.DataProperty("universeSize", 100))
additionalData.addProperty(DATA.DataProperty("itemName", "foo"))

# Create an empty Policy
policy = POL.PolicyPtr()

# Get a Persistence object
persistence = PERS.Persistence.getPersistence(policy)

# Set up the LogicalLocation.  Assumes that previous tests have run, and
# Src_*.fits exists in the current directory.
logicalLocation = PERS.LogicalLocation("Src_img.fits")

# Create a FitsStorage and put it in a StorageList.
storage = persistence.getRetrieveStorage("FitsStorage", logicalLocation)
storageList = PERS.StorageList([storage])

# Let's do the retrieval!
persPtr = persistence.unsafeRetrieve("ImageF", storageList, additionalData)
image = FWCL.ImageF.swigConvert(persPtr)

# Check the resulting Image
# ...

# Persist the Image (under a different name, and in a different format)
logicalLocation = PERS.LogicalLocation("image.boost")
storage = persistence.getPersistStorage("BoostStorage", logicalLocation)
storageList = PERS.StorageList([storage])
persistence.persist(image, storageList, additionalData)

# Retrieve it again
storage = persistence.getRetrieveStorage("BoostStorage", logicalLocation)
storageList = PERS.StorageList([storage])
pers2Ptr = persistence.unsafeRetrieve("ImageF", storageList, additionalData)
image2 = FWCL.ImageF.swigConvert(pers2Ptr)

# Check to make sure that we got the same data
assert image.getRows() == image2.getRows()
assert image.getCols() == image2.getCols()
assert image.getOffsetRows() == image2.getOffsetRows()
assert image.getOffsetCols() == image2.getOffsetCols()
for c in xrange(image.getCols()):
    for r in xrange(image.getRows()):
        pixel1 = image.getPtr(c, r)
        pixel2 = image2.getPtr(c, r)
        # Persisting through Boost text archives causes conversion error!
        # assert abs(pixel1 - pixel2) / pixel1 < 1e-7, \
        assert pixel1 == pixel2, \
                "Differing pixel2 at %d, %d: %f, %f" % (c, r, pixel1, pixel2)
