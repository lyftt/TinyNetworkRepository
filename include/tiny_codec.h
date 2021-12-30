#ifndef __CODEC_H__
#define __CODEC_H__

#include <cstddef>

struct Msg
{

};

struct CodeBase
{
    virtual void encode(const Msg& msg) = 0;
    virtual Msg decode(char* buf, size_t len) = 0;
    virtual ~CodeBase() {}
};

#endif