#include "objnetdevice.h"

using namespace Objnet;

ObjnetDevice::ObjnetDevice(unsigned char netaddr) :
    mClassValid(false),
    mNameValid(false),
    mNetAddress(netaddr),
    mPresent(false),
    mTimeout(5),
    mAutoDelete(false)
//    mStateChanged(false),
//    mOrphanCount(0),
//    mChildrenCount(0)
{
    //mTimer.start();
}
