import gdb
import math, re

try:
    debug
except:
    debug = False
    
try:
    import gdb.printing

    class SharedPtrPrinter(object):
        "Print a shared_ptr"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            if self.val["px"]:
                return "shared_ptr(%s)" % self.val["px"].dereference()
            else:
                return "NULL"

    #-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    def getEigenMatrixDimensions(val):
        m_storage = val["m_storage"]
        try:
            nx, ny = m_storage["m_cols"], m_storage["m_rows"]
        except gdb.error:           # only available for dynamic Matrices
            try:
                nx, ny = m_storage.type.template_argument(1), m_storage.type.template_argument(2)
            except RuntimeError:
                # should get dimens from template, but that's gdb bug #11060
                size = m_storage["m_data"]["array"].type.sizeof
                size0 = m_storage["m_data"]["array"].dereference().type.sizeof
                # guess! Assume square
                nx = int(math.sqrt(size/size0))
                ny = size/(nx*size0)

        return nx, ny

    def getEigenValue(var, x, y=0):
        if re.search(r"Matrix", str(var.type)):
            if False:
                return var["operator()(int, int)"](x, y)
            else:
                NX, NY = getEigenMatrixDimensions(var)

                if x < 0 or x >= NX or y < 0 or y >= NY:
                    raise gdb.GdbError("Element (%d, %d) is out of range 0:%d, 0:%d" %
                                       (x, y, NX - 1, NY - 1))

                m_data = var["m_storage"]["m_data"]
                if False:
                    # convert to a pointer to the start of the array
                    import pdb; pdb.set_trace() 
                    m_data = m_data.address.cast(m_data.type)

                try:
                    val = m_data[x + y*NX]
                except:
                    val = m_data["array"][x + y*NX]
        else:                       # Vector
            if x < 0 or x >= m_storage["m_cols"]:
                raise gdb.GdbError("Element %d is out of range 0:%d" % (x, m_storage["m_cols"] - 1))

            val = m_storage["m_data"][x]

        if val.type.code == gdb.TYPE_CODE_INT:
            val = int(val)
        elif val.type.code == gdb.TYPE_CODE_FLT:
            val = float(val)

        return val            

    class EigenMatrixPrinter(object):
        "Print an Eigen Matrix"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            nx, ny = getEigenMatrixDimensions(self.val)

            return "%s{%dx%d}" % (self.val.type, nx, ny)

    class EigenVectorPrinter(object):
        "Print an Eigen Vector"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            m_storage = self.val["m_storage"]

            try:
                n = m_storage["n"]
            except gdb.error:           # only available for dynamic Matrices
                try:
                    n = m_storage.type.template_argument(1)
                except RuntimeError:
                    # should get dimens from template, but that's gdb bug #11060
                    size = m_storage["m_data"]["array"].type.sizeof
                    size0 = m_storage["m_data"]["array"].dereference().type.sizeof
                    n = math.sqrt(size/size0)

            return "{%d}" % (n)

    class PrintEigenCommand(gdb.Command):
        """Print an eigen Matrix or Vector
    Usage: show eigen <matrix> [x0 y0 [nx ny]]
           show eigen <vector> [x0 [nx]]
    """

        def __init__ (self):
            super (PrintEigenCommand, self).__init__ ("show eigen",
                                                      gdb.COMMAND_DATA,
                                                      gdb.COMPLETE_SYMBOL)


        def _mget(self, var, x, y=0):
            return getEigenValue(var, x, y)

        def _vget(self, var, x):
            return getEigenValue(var, x)

        def invoke (self, args, fromTty):
            self.dont_repeat()

            args = gdb.string_to_argv(args)
            if len(args) < 1:
                raise gdb.GdbError("Please specify a matrix or vector")
            imgName = args.pop(0)
            var = gdb.parse_and_eval(imgName)

            if not re.search(r"^Eigen::(Matrix|Vector)", str(var.type)):
                raise gdb.GdbError("Please specify an eigen matrix or vector, not %s" % var.type)
                
            if re.search(r"shared_ptr<", str(var.type)):
                var = var["px"].dereference()

            if var.type.code == gdb.TYPE_CODE_PTR:
                var = var.dereference()     # be nice

            isMatrix = re.search(r"Matrix", str(var.type))
            if isMatrix:
                x0, y0 = 0, 0
                NX, NY = getEigenMatrixDimensions(var)
                nx, ny = NX, NY

                if args:
                    if len(args) == 1:
                        raise gdb.GdbError("Please specify an element's x and y indexes")
                    else:
                        x0 = gdb.parse_and_eval(args.pop(0))
                        y0 = gdb.parse_and_eval(args.pop(0))

                if args:
                    nx = int(args.pop(0))
                    if args:
                        ny = int(args.pop(0))
                    else:
                        ny = 1

                if nx == 1 and ny == 1:
                    print "%g" % self._vget(var, x0, y0)
                    return
            else:
                x0, nx = 0, var["m_storage"]["n"]

                if args:
                    x0 = gdb.parse_and_eval(args.pop(0))
                if args:
                    nx = int(args.pop(0))

                if nx == 1:
                    print "%g" % self._vget(var, x0)
                    return

            if args:
                raise gdb.GdbError('Unexpected trailing arguments: "%s"' % '", "'.join(args))
            #
            # OK, finally time to print
            #
            dataFmt = "%.2f"

            print "%-4s" % "",
            for x in range(x0, min(NX, x0 + nx)):
                print "%8d" % x,
            print ""

            if isMatrix:
                for y in range(y0, min(NY, y0 + ny)):
                    print "%-4d" % y,
                    for x in range(x0, min(NX, x0 + nx)):
                        print "%8s" % (dataFmt % self._mget(var, x, y)),
                    print ""
            else:
                for x in range(x0, min(NX, x0 + nx)):
                    print "%8s" % (dataFmt % self._vget(var, x)),
                print ""

    PrintEigenCommand()

    #-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    class CitizenPrinter(object):
        "Print a Citizen"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            sentinel = long(self.val["_sentinel"].cast(gdb.lookup_type("unsigned int")))
            return "{%s %d 0x%x}" % (self.val.address, self.val["_CitizenId"], sentinel)

    class PrintCitizenCommand(gdb.Command):
        """Print a Citizen
    Usage: show citizen <obj>
    """

        def __init__ (self):
            super (PrintCitizenCommand, self).__init__ ("show citizen",
                                                        gdb.COMMAND_DATA,
                                                        gdb.COMPLETE_SYMBOL)

        def invoke (self, args, fromTty):
            self.dont_repeat()

            args = gdb.string_to_argv(args)
            if len(args) < 1:
                raise gdb.GdbError("Please specify an object")
            objName = args.pop(0)
            
            if args:
                raise gdb.GdbError('Unexpected trailing arguments: "%s"' % '", "'.join(args))

            var = gdb.parse_and_eval(objName)

            if var.type.code != gdb.TYPE_CODE_PTR:
                raise gdb.GdbError("%s it not a pointer" % objName)

            try:
                citizen = var.dynamic_cast(gdb.lookup_type("lsst::daf::base::Citizen").pointer()).dereference()
            except gdb.error:
                raise gdb.GdbError("Failed to cast %s to Citizen *" % objName)

            print citizen

    PrintCitizenCommand()

    #-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    # afw

    class BaseSourceAttributesPrinter(object):
        "Print a BaseSourceAttributes"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            return "Base: {id=%d astrom=(%.3f, %.3f)}" % (self.val["_id"], self.val["_xAstrom"], self.val["_yAstrom"])

    class SourcePrinter(object):
        "Print a Source"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            return "Source{id=%d astrom=(%.3f, %.3f)}" % (self.val["_id"],
                                                          self.val["_xAstrom"], self.val["_yAstrom"])

    class FootprintPrinter(object):
        "Print a Footprint"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            if False:
                nspan = self.val["_spans"]["size"]() # Fails (as it's type is METHOD, not CODE)
            else:
                vec_impl = self.val["_spans"]["_M_impl"]
                nspan = vec_impl["_M_finish"] - vec_impl["_M_start"]

            return "Footprint{id=%d, nspan=%d, area=%d; BBox %s}" % (self.val["_fid"], nspan,
                                                                     self.val["_area"], self.val["_bbox"])

    class FootprintSetPrinter(object):
        "Print a FootprintSet"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            return "FootprintSet{%s; %s}" % (self.val["_region"], self.val["_footprints"])

    class PeakPrinter(object):
        "Print a Peak"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            return "Peak{%d, (%d, %d), (%.3f, %.3f)}" % (self.val["_id"], self.val["_ix"], self.val["_iy"],
                                                         self.val["_fx"], self.val["_fy"])

    #-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    class Box2Printer(object):
        "Print a Box2"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            # Make sure &foo works, too.
            type = self.val.type
            if type.code == gdb.TYPE_CODE_REF:
                type = type.target ()

            llc = [getEigenValue(self.val["_minimum"]["_vector"], 0, i) for i in range(2)]
            dims = [getEigenValue(self.val["_dimensions"]["_vector"], 0, i) for i in range(2)]

            return "Box2{(%s,%s)--(%s,%s)}" % (llc[0], llc[1],
                                                 llc[0] + dims[0] - 1, llc[1] + dims[1] - 1)

        def display_hint (self):
            return "array"

    class CoordinateBasePrinter(object):
        "Print a CoordinateBase"

        def __init__(self, val):
            self.val = val

        def to_string(self):
            # Make sure &foo works, too.
            type = self.val.type
            if type.code == gdb.TYPE_CODE_REF:
                type = type.target ()

            return self.val["_vector"]["m_storage"]["m_data"]["array"]

        def display_hint (self):
            return "array"

    #-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    class ImagePrinter(object):
        "Print an ImageBase or derived class"

        def dimenStr(self, val=None):
            if val is None:
                val = self.val

            # Make sure &foo works, too.
            type = val.type
            if type.code == gdb.TYPE_CODE_REF:
                type = type.target()

            gilView = val["_gilView"]
            arr = val["_origin"]["_vector"]["m_storage"]["m_data"]["array"]

            x0, y0 = arr[0], arr[1]
            return "%dx%d%s%d%s%d" % (
                #val["getWidth"](), val["getHeight"](), 
                gilView["_dimensions"]["x"], gilView["_dimensions"]["y"],
                ["", "+"][x0 >= 0], x0, # i.e. "+" if x0 >= 0 else "" in python >= 2.5
                ["", "+"][y0 >= 0], y0)

        def typeName(self):
            return self.typename.split(":")[-1]

        def __init__(self, val):
            self.typename = str(val.type)
            self.val = val

        def to_string(self):
            return "%s(%s)" % (self.typeName(), self.dimenStr())

    class MaskedImagePrinter(ImagePrinter):
        "Print a MaskedImage"

        def to_string(self):
            return "%s(%s)" % (self.typeName(), self.dimenStr(self.val["_image"]["px"].dereference()))

    class ExposurePrinter(ImagePrinter):
        "Print an Exposure"

        def to_string(self):
            return "%s(%s)" % (self.typeName(),
                               self.dimenStr(self.val["_maskedImage"]["_image"]["px"].dereference()))

    #-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    class PrintImageCommand(gdb.Command):
        """Print an Image
    Usage: show image <image> x0 y0 [nx [ny] [centerPatch] [obeyXY0]]
    If nx or ny is 0, show the entire image
    """

        def __init__ (self):
            super (PrintImageCommand, self).__init__ ("show image",
                                                      gdb.COMMAND_DATA,
                                                      gdb.COMPLETE_SYMBOL)

        def get(self, var, x, y):
            if False:
                return var["operator()(int, int, bool)"](x, y, True)
            else:
                dimensions = var["_gilView"]["_dimensions"]
                if x < 0 or x >= dimensions["x"] or y < 0 or y >= dimensions["y"]:
                    raise gdb.GdbError("Pixel (%d, %d) is out of range 0:%d, 0:%d" %
                                       (x, y, dimensions["x"] - 1, dimensions["y"] - 1))

                pixels = var["_gilView"]["_pixels"]["_p"]
                step = pixels["_step_fn"]["_step"]/var.type.template_argument(0).sizeof

                return pixels["m_iterator"][x + y*step]["_v0"]

        def invoke (self, args, fromTty):
            self.dont_repeat()

            args = gdb.string_to_argv(args)
            if len(args) < 1:
                raise gdb.GdbError("Please specify an image")
            imgName = args.pop(0)
            var = gdb.parse_and_eval(imgName)

            if re.search(r"shared_ptr<", str(var.type)):
                var = var["px"].dereference()

            if not re.search(r"^lsst::afw::image::(Image|Mask|MaskedImage)", str(var.type)):
                raise gdb.GdbError("Please specify an image, not %s" % var.type)

            if re.search(r"MaskedImage", str(var.type)):
                print "N.b. %s is a MaskedImage; showing image" % (imgName)
                var = var["_image"]

            if re.search(r"shared_ptr<", str(var.type)):
                var = var["px"].dereference()

            if var.type.code == gdb.TYPE_CODE_PTR:
                var = var.dereference()     # be nice

            pixelTypeName = str(var.type.template_argument(0))
            if pixelTypeName in ["short", "unsigned short"]:
                dataFmt = "0x%x"
            elif pixelTypeName in ["int", "unsigned int"]:
                dataFmt = "%d"
            else:
                dataFmt = "%.2f"

            if len(args) == 1 and args[0] == "all":
                args.pop(0)
                x0, y0 = 0, 0
                nx, ny = 0, 0
            else:
                if len(args) < 2:
                    raise gdb.GdbError("Please specify a pixel's x and y indexes")

                x0 = gdb.parse_and_eval(args.pop(0))
                y0 = gdb.parse_and_eval(args.pop(0))

                if len(args) == 0:
                    print dataFmt % self.get(var, x0, y0)
                    return

                nx = int(args.pop(0))
                if args:
                    ny = int(args.pop(0))
                else:
                    ny = 1

            if nx == 0:
                nx = var["_gilView"]["_dimensions"]["x"]
            if ny == 0:
                ny = var["_gilView"]["_dimensions"]["y"]

            if args:
                centerPatch = gdb.parse_and_eval(args.pop(0))
                if centerPatch:
                    x0 -= nx//2
                    y0 -= ny//2

            if args:
                obeyXY0 = gdb.parse_and_eval(args.pop(0))

                if obeyXY0:
                    arr = var["_origin"]["_vector"]["m_storage"]["m_data"]["array"]

                    x0 -= arr[0]
                    y0 -= arr[1]

            if args:
                raise gdb.GdbError('Unexpected trailing arguments: "%s"' % '", "'.join(args))
            #
            # OK, finally time to print
            #
            print "%-4s" % "",
            for x in range(x0, x0 + nx):
                print "%8d" % x,
            print ""

            for y in reversed(range(y0, y0 + ny)):
                print "%-4d" % y,
                for x in range(x0, x0 + nx):
                    print "%8s" % (dataFmt % self.get(var, x, y)),
                print ""


    PrintImageCommand()

    #-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    printers = []

    def register(obj):
        "Register my pretty-printers with objfile Obj."

        if obj is None:
            obj = gdb

        for p in printers:
            gdb.printing.register_pretty_printer(obj, p)

    #-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    def build_boost_dictionary():
        """Surely this must be somewhere standard?"""
        
        printer = gdb.printing.RegexpCollectionPrettyPrinter("rhl-boost")

        printer.add_printer('boost::shared_ptr',
                            '^(boost|tr1|std)::shared_ptr', SharedPtrPrinter)

        return printer

    printers.append(build_boost_dictionary())

    def build_eigen_dictionary():
        """Surely this must be somewhere standard?"""
        
        printer = gdb.printing.RegexpCollectionPrettyPrinter("rhl-eigen")

        printer.add_printer('eigen::Matrix',
                            '^Eigen::Matrix', EigenMatrixPrinter)
        printer.add_printer('eigen::Vector',
                            '^Eigen::Vector', EigenVectorPrinter)

        return printer

    printers.append(build_eigen_dictionary())
       
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    def build_afw_dictionary():
        printer = gdb.printing.RegexpCollectionPrettyPrinter("afw")

        printer.add_printer('lsst::afw::detection::Footprint',
                            '^lsst::afw::detection::Footprint$', FootprintPrinter)
        printer.add_printer('lsst::afw::detection::FootprintSet',
                            '^lsst::afw::detection::FootprintSet', FootprintSetPrinter)
        printer.add_printer('lsst::afw::detection::Peak',
                            '^lsst::afw::detection::Peak$', PeakPrinter)
        printer.add_printer('lsst::afw::detection::Source',
                            '^lsst::afw::detection::Source$', SourcePrinter)
        printer.add_printer('lsst::afw::detection::BaseSourceAttributes',
                            '^lsst::afw::detection::BaseSourceAttributes$', BaseSourceAttributesPrinter)

        printer.add_printer('lsst::afw::geom::Box',
                            '^lsst::afw::geom::Box', Box2Printer)
        printer.add_printer('lsst::afw::geom::Extent',
                            '^lsst::afw::geom::Extent', CoordinateBasePrinter)
        printer.add_printer('lsst::afw::geom::Point',
                            '^lsst::afw::geom::Point', CoordinateBasePrinter)

        printer.add_printer('lsst::afw::image::ImageBase',
                            'lsst::afw::image::ImageBase<[^>]+>$', ImagePrinter)
        printer.add_printer('lsst::afw::image::Image',
                            'lsst::afw::image::Image<[^>]+>$', ImagePrinter)
        printer.add_printer('lsst::afw::image::Mask',
                            '^lsst::afw::image::Mask<[^>]+>$', ImagePrinter)
        printer.add_printer('lsst::afw::image::MaskedImage',
                            '^lsst::afw::image::MaskedImage<[^>]+>$', MaskedImagePrinter)
        printer.add_printer('lsst::afw::image::Exposure',
                            '^lsst::afw::image::Exposure', ExposurePrinter)

        return printer

    printers.append(build_afw_dictionary())

    def build_daf_base_dictionary():
        printer = gdb.printing.RegexpCollectionPrettyPrinter("daf::base")

        printer.add_printer('lsst::daf::base::Citizen',
                            'lsst::daf::base::Citizen', CitizenPrinter)

        return printer

    printers.append(build_daf_base_dictionary())
except ImportError:
    from printers_oldgdb import *
