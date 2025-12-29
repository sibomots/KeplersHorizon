#ifndef __ICMD_H__
#define __ICMD_H__

class ICmd
{
  public:
    virtual ~ICmd() = default;
    virtual bool invoke() = 0;
};

#endif
