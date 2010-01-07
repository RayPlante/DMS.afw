#include "lsst/afw/geom/CoordinateBase.h"
#include "lsst/afw/geom/Point.h"
#include "lsst/afw/geom/Extent.h"

namespace geom = lsst::afw::geom;

template <int N>
geom::CoordinateExpr<N> geom::CoordinateExpr<N>::and_(CoordinateExpr<N> const & other) const {
    CoordinateExpr r(*this);
    for (register int n=0; n<N; ++n) {
        if (!other[n]) r[n] = false;
    }
    return r;
}

template <int N>
geom::CoordinateExpr<N> geom::CoordinateExpr<N>::or_(CoordinateExpr<N> const & other) const {
    CoordinateExpr r(*this);
    for (register int n=0; n<N; ++n) {
        if (other[n]) r[n] = true;
    }
    return r;
}

template <int N>
geom::CoordinateExpr<N> geom::CoordinateExpr<N>::not_() const {
    CoordinateExpr r;
    for (register int n=0; n<N; ++n) {
        if (!this->operator[](n)) r[n] = true;
    }
    return r;
}

template class geom::CoordinateExpr<2>;
template class geom::CoordinateExpr<3>;
