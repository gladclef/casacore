//# RecordRep.h: The representation of a Record
//# Copyright (C) 1996,1997,2000,2001,2005
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
//#
//# $Id$


#ifndef CASA_SERIALIZEHELPER_H
#define CASA_SERIALIZEHELPER_H

//# Includes
#include <casacore/casa/IO/MemoryIO.h>
#include <casacore/casa/Exceptions/Error.h>
#include <casacore/casa/aipsxtype.h>
#include <climits>
#include <map>

namespace casacore { //# NAMESPACE CASACORE - BEGIN

// <summary>
// Helper class, as used by other classes to serialize/deserialize
// in pieces. The intended use case is serializing a large object
// without needing two copies of the object in memory at a time, one
// of the object and a duplicate in a contiguous array waiting to
// be written to disk/network/etc.
// </summary>
//
// <synopsis>
// 
// </synopsis>
//
// <example>
// // Write ~1000 bytes at a time to a file stream
// MemoryIO buf(2000);
// AipsIO rBuf(&buf);
// FILE *fout = fopen('out.rec', 'wb');
// SerializeHelper sh(1000, buf);
// do {
//     sh.reset();
//     record.putData(rBuf, &sh);
//     fwrite(fout, buf.data(), buf.length());
// } while (buf.length() > 0);
// fclose(fout);
// </example>
//
// <example>
// // Use a SerializeHelper to serialize your data.
// void RecordRep::putData(AipsIO &os, SerializeHelper *sh) {
//     sh = SerializeHelper::getInstance(sh, this);
//     Int64 idx = SerializeHelper::getIndex(sh);
//     Int64 avail = SerializeHelper::update(sh, idx);
//     if (avail <= 0) return;

//     if (idx <= 0) {
//         os << itsHeader;
//         idx = 1;
//         avail = SerializeHelper::update(sh, idx);
//     }

//     Int64 i;
//     for (i = (idx-1); i < ndata && avail > 0; i++) {
//         putDataField(os, datum(i));
//         SerializeHelper::complete(sh, &(datum(i)) );
//         idx = i+1;
//         avail = SerializeHelper::update(sh, idx);
//     }

//     if (idx < 0 || idx == ndata+1) {
//         os << itsFooter;
//         idx = ndata+2;
//         SerializeHelper::update(sh, idx);
//     }
// }
// </example>
//
// <motivation>
// Reduce memory usage.
// Reduce memory read/write transfers overhead.
// </motivation>


class SerializeHelper
{
public:
    // Create a serializer helper with a soft size limit.
    // The "hard limit" to check against during serialization is twice the soft limit. // TODO remove hard limit
    // The associated buf will be cleared when calling reset on this instance.
    SerializeHelper(Int64 softLimit, MemoryIO &buf);

    // The origin SerializeHelper should free all the instances when destructed.
    ~SerializeHelper();

    static int getLogLevel();

    // Get the SerializeHelper instance for the current object.
    // If parent is NULL, return NULL.
    // If the parent is not NULL and object isn't registered yet, then register a new
    // instance to the object and return it.
    // Idx is set to the current index for the returned instance (-1 if parent is NULL).
    // Avail is set to the current available byte count for the returned instance (LLONG_MAX if parent is NULL).
    static SerializeHelper *getInstance(SerializeHelper *parent, void *object, Int64& idx, Int64& avail);

    // Get the current writing index for the given instance, if any.
    // Returns -1 by default if instance is NULL;
    static Int64 getIndex(SerializeHelper *instance);

    // Get the current available bytes for the given instance, if any.
    // Returns LLONG_MAX by default if instance is NULL;
    static Int64 getAvailable(SerializeHelper *instance);

    // Lets the SerializeHelper know that data has been written to the buf.
    // Updates the number of bytes available in the buf.
    // Updates the index in the instance.
    // Returns the number of bytes available in the buf. If instance is NULL, returns MAX_INT.
    static Int64 update(SerializeHelper *instance, Int64 index);

    // Remove the SerializeHelper instances associated with the given object,
    // effectively resetting the serialization count for that object.
    static void objectSerialized(SerializeHelper *instance, void *object);

    // Instance method, to be called by static method, typically.
    // This method can be called explicity when the calling code is sure that
    // the SerializeHelper instance in use is not NULL.
    Int64 update(Int64 index);

    // Clears the MemoryIO buffer, to make more space available without actually
    // using any more memory.
    // This effectively raising the available memory back to its soft limit.
    void clearMemory();

protected:
    // Instance methods, to be called by static method.
    SerializeHelper *getInstance(void *object, Int64& idx, Int64& avail);
    void objectSerialized(void *object);

    // Return the available number of bytes for this instance.
    Int64 getAvailable();

    // The current index for this instance. Should only ever increase in value.
    Int64 index;

    // The primary helper, which tracks the instances register.
    SerializeHelper *origin;
    
    // Limits on how much can be written.
    // softLimit informs the returned "available bytes" from update().
    Int64 softLimit;

    // The buffer to check for the number of used bytes, and to clear
    // when reset is called.
    MemoryIO &buf;

    // The list of instances, indexed by the objects that access them.
    std::map<void*, SerializeHelper*> instances;
};



} //# NAMESPACE CASACORE - END

#endif
