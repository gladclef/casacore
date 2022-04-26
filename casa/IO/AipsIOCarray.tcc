//# AipsIOCarray.cc:  Templated functions to get/put a C-array from/into AipsIO.
//# Copyright (C) 1993,1994,1995,1996,2001
//# Associated Universities, Inc. Washington DC, USA.
//# 
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//# 
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//# 
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//# 
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$

#ifndef CASA_AIPSIOCARRAY_TCC
#define CASA_AIPSIOCARRAY_TCC

#include <casacore/casa/IO/AipsIOCarray.h>
#include <casacore/casa/IO/MemoryIO.h>
#include <casacore/casa/Exceptions/Error.h>
#include <casacore/casa/Utilities/Assert.h>

namespace casacore { //# NAMESPACE CASACORE - BEGIN

// Put a C-array of n elements.
template<class T>
void putAipsIO (AipsIO& ios, uInt n, const T* data, SerializeHelper *sh, Int64 shStartIdx)
{
    int ll = SerializeHelper::getLogLevel();
    // sh = SerializeHelper::getInstance(sh, &a);
    Int64 idx = SerializeHelper::getIndex(sh);
    Int64 avail = SerializeHelper::getAvailable(sh);
    if (ll >= 2) std::cerr << "..ArrayIOCarray::putAipsIO (sh: " << sh << ", idx: " << idx << ", avail: " << avail << ")" << std::endl;
    if (avail <= 0) return;

    if (sh != NULL) {
        // write the put count
        // shStartIdx starts us indexing at 1, from ArrayIO::putArrayPart
        if (idx <= shStartIdx) {
            if (ll >= 3) std::cerr << "..ArrayIOCarray::putAipsIO (" << sh << ") put count" << std::endl;
            ios << n;
            idx = shStartIdx+1;
            avail = sh->update(idx);
        }

        // skip to the current data
        uInt i, start = (uInt)idx - shStartIdx - 1;
        if (ll >= 3) std::cerr << "..ArrayIOCarray::putAipsIO (" << sh << ") skip to " << start << std::endl;
        data += start;

        // write the data
        if (ll >= 3) std::cerr << "..ArrayIOCarray::putAipsIO (" << sh << ") put data" << std::endl;
        for (i = start; i < n && avail > 0; i++, data++) {
            ios << *data;
            idx = (Int64)i + shStartIdx + 2;
            if (i % 100 == 0) {
                avail = sh->update(idx);
            }
        }
        avail = sh->update(idx);

        // update the index to indicate that all writing has been done
        if (i == n) {
            if (ll >= 3) std::cerr << "..ArrayIOCarray::putAipsIO (" << sh << ") done" << std::endl;
            sh->update(LLONG_MAX-1);
        }
    } else {
        // optimization for when we don't have a SerializeHelper
        ios << n;
        for (uInt i = 0; i < n; i++, data++) {
            ios << *data;
        }
    }
}

// Get n elements into an already available C-array.
template<class T>
void getAipsIO (AipsIO& ios, uInt n, T* data)
{
    for (uInt i=0; i<n; i++) {
	ios >> *data++;
    }
}

// Get elements into a C-array to be allocated on the heap.
// The number of elements will also be returned.
template<class T>
void getnewAipsIO (AipsIO& ios, uInt& n, T** data)
{
    ios >> n;
    *data = new T[n];
    getAipsIO (ios, n, *data);
}

} //# NAMESPACE CASACORE - END


#endif
