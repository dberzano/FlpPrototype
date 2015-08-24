/**
 * O2FLPex.h
 *
 * @since 2014-02-24
 * @author A. Rybalchenko
 */

#ifndef O2FLPEX_H_
#define O2FLPEX_H_

#include <string>

#include "FairMQDevice.h"
#include "DataBlock.h"

struct Content {
  int id;
  double a;
  double b;
  int x;
  int y;
  int z;
};

class O2FLPex : public FairMQDevice
{
  public:
    enum {
      InputFile = FairMQDevice::Last,
      EventSize,
      Last
    };
    O2FLPex();
    virtual ~O2FLPex();
    void Log(int intervalInMs);

    virtual void SetProperty(const int key, const std::string& value);
    virtual std::string GetProperty(const int key, const std::string& default_ = "");
    virtual void SetProperty(const int key, const int value);
    virtual int GetProperty(const int key, const int default_ = 0);

  protected:
    int fEventSize;
    char *payload;
    virtual void Init();
    virtual void Run();
};

#endif
